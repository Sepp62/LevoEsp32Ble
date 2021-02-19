/** Levo ESP32 BLE Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 *  Derived from NimBLE client sample: H2zero: https://github.com/h2zero/NimBLE-Arduino
 * 
 *  Contains all functionality to establish, maintain and control bluetooth connection to the Levo bike
 *  Implements notification receiver and data decoder.
 *  Basic functions for read and write. Use class "LevoReadWrite" for high level read/write access.
 * 
 */

#include "LevoEsp32Ble.h"

// bluetooth pin
uint32_t LevoEsp32Ble::m_pin = 0L;

// connection status
LevoEsp32Ble::enBleStatus LevoEsp32Ble::m_bleStatus = LevoEsp32Ble::OFFLINE;

// ble connect "state machine
bool     LevoEsp32Ble::m_doConnect = false;
uint32_t LevoEsp32Ble::m_scanTime = 0; /** 0 = scan forever */
bool     LevoEsp32Ble::m_queueOverrun = false;
bool     LevoEsp32Ble::m_bAutoReconnect = true;
bool     LevoEsp32Ble::m_bSubscribed = false;

// ble message queue from bluetooth stack running on core 0 to main thread on core 1
QueueHandle_t LevoEsp32Ble::m_bleMsgQueue;

void scanEndedCB(NimBLEScanResults results);

static NimBLEAdvertisedDevice* s_advDevice = 0;

class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient* pClient)
    {
        LevoEsp32Ble::m_bleStatus = LevoEsp32Ble::CONNECTED;
        Serial.println("Connected");
        // After connection we should change the parameters if we don't need fast response times.
        // Levo requests this timing: interval: min/max 20/40, latency: 4, timeout: 4000ms
        pClient->updateConnParams(20,40,4,400);
    };

    void onDisconnect(NimBLEClient* pClient)
    {
        LevoEsp32Ble::m_bleStatus = LevoEsp32Ble::OFFLINE;
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
        if(LevoEsp32Ble::m_bAutoReconnect )
            NimBLEDevice::getScan()->start(LevoEsp32Ble::m_scanTime, scanEndedCB);
    };
    
    /** Called when the peripheral requests a change to the connection parameters.
     *  Return true to accept and apply them or false to reject and keep 
     *  the currently used parameters. Default will return true.
     */
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params)
    {
        // accept changed timing, "return false" is not liked by Levo
        // Serial.print("onConnParamsUpdateRequest(min,max,lat,to):");
        // Serial.print(params->itvl_min); Serial.print(", "); // in 1.25ms units
        // Serial.print(params->itvl_max); Serial.print(", "); // in 1.25ms units
        // Serial.print(params->latency); Serial.print(", ");
        // Serial.println(params->supervision_timeout);  // in 10ms
        return true;
    };
    
    //********************* Security handled here **********************
    uint32_t onPassKeyRequest()
    {
        Serial.println("Client Passkey Request");
        // return the passkey to send to the server
        return LevoEsp32Ble::m_pin; // PASS_KEY;
    };

    // Pairing process complete, we can check the results in ble_gap_conn_desc
    void onAuthenticationComplete(ble_gap_conn_desc* desc)
    {
        if(!desc->sec_state.encrypted)
        {
            LevoEsp32Ble::m_bleStatus = LevoEsp32Ble::AUTHERROR;
            Serial.println("Encrypt connection failed - disconnecting");
            // Find the client with the connection handle provided in desc
            NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
            return;
        }
        Serial.println("Encrypt connection succeeded");
    };
};

// Define a class to handle the callbacks when advertisments are received
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks
{
    void onResult(NimBLEAdvertisedDevice* advertisedDevice)
    {
        Serial.print("Advertised Device found: ");
        Serial.println(advertisedDevice->toString().c_str());
        // Check for our Levo
        // manufacturer data: 0x0059 (Nordic)  "TURBOHMI2017" - 5900545552424f484d493230313701000000
        if (advertisedDevice->haveName())
        {
            // We have found a device, let us now see if it contains the service we are looking for.
            std::string man = advertisedDevice->getManufacturerData();
            if (man.length() > 10 && man.compare(2, 8, "TURBOHMI") == 0)
            {
                Serial.println("Found our device!");
                advertisedDevice->getScan()->stop();
                s_advDevice = advertisedDevice;
                LevoEsp32Ble::m_doConnect = true;
                LevoEsp32Ble::m_bleStatus = LevoEsp32Ble::CONNECTING;
            }
        } // Found our server
    };
};

// Notification / Indication receiving handler callback
// runs in core 0 --> xPortGetCoreID()
// see https://savjee.be/2020/01/multitasking-esp32-arduino-freertos/
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    // check for correct characteristics uuid
    std::string rc(pRemoteCharacteristic->getUUID());
    if (rc.compare(LEVO_DATA_CHAR_UUID_STRING) != 0)
        return;

    // send ble value to main thread on core1
    LevoEsp32Ble::stBleMessage bleMsg;
    bleMsg.len = length;
    size_t minLen = min(length, sizeof(bleMsg.data));
    memcpy(bleMsg.data, pData, minLen);
    if (xQueueSend(LevoEsp32Ble::m_bleMsgQueue, &bleMsg, 0) == errQUEUE_FULL)
    {
        LevoEsp32Ble::m_queueOverrun = true;
        Serial.println( "Ble queue overrun" );
    }
}

// Callback to process the results of the last scan or restart it
void scanEndedCB(NimBLEScanResults results)
{
    Serial.println("Scan Ended");
}

// Create a single global instance of the callback class to be used by all clients
static ClientCallbacks clientCB;

// get int value from buffer
float LevoEsp32Ble::int2float(uint8_t* pData, size_t length, size_t valueSize, int bufOffset )
{
    uint32_t val = 0;

    // autosize, assume value needs whole buffer
    if( valueSize == -1 )
        valueSize = length - bufOffset;

    // check buffer length
    if( (bufOffset + valueSize) > length )
        valueSize = length - bufOffset;

    // get integer value
    if (valueSize == 1)
        val = pData[2];
    else if (valueSize == 2)
        val = pData[2] + (((uint32_t)(pData[3])) << 8);
    else if (valueSize == 4)
        val = pData[2] + (((uint32_t)(pData[3])) << 8) + (((uint32_t)(pData[4])) << 16) + (((uint32_t)(pData[5])) << 24);

    return (float)val;
}


// decode our BLE message to get Levo data
//////////////////////////////////////////
bool LevoEsp32Ble::DecodeMessage(uint8_t* pData, size_t length, stBleVal & bleVal )
{
    bleVal.dataType  = LevoEsp32Ble::UNKNOWN;
    bleVal.unionType = FLOAT;

    uint8_t  sender  = pData[0];
    uint8_t  channel = pData[1];

    switch (sender)
    {
    case 0: // battery
        switch (channel)
        {
        case 0:  bleVal.dataType = BATT_SIZEWH;         bleVal.fVal = round(int2float(pData, length, 2) * 1.1111f); break; // 00 00 c2 01 450Wh * 1.1111
        case 1:  bleVal.dataType = BATT_REMAINWH;       bleVal.fVal = round(int2float(pData, length, 2) * 1.1111f); break; // 00 01 e4 00 full= 450Wh*1.1111
        case 2:  bleVal.dataType = BATT_HEALTH;         bleVal.fVal = int2float(pData, length, 1); break;                  // 00 02 64
        case 3:  bleVal.dataType = BATT_TEMP;           bleVal.fVal = int2float(pData, length, 1); break;                  // 00 03 13
        case 4:  bleVal.dataType = BATT_CHARGECYCLES;   bleVal.fVal = int2float(pData, length, 2); break;                  // 00 04 0d 00
        case 5:  bleVal.dataType = BATT_VOLTAGE;        bleVal.fVal = int2float(pData, length, 1)/10.0f + 28.0f; break;    // 00 05 50
        case 6:  bleVal.dataType = BATT_CURRENT;        bleVal.fVal = int2float(pData, length, 1)/10.0f; break;            // 00 06 00
        case 12: bleVal.dataType = BATT_CHARGEPERCENT;  bleVal.fVal = int2float(pData, length, 1); break;                  // 00 0c 34
        }
        break;
    case 1: // motor
        switch (channel)
        {
        case 0:  bleVal.dataType = RIDER_POWER;     bleVal.fVal = int2float(pData, length, 2);         break;     // 01 00 00 00
        case 1:  bleVal.dataType = MOT_CADENCE;     bleVal.fVal = int2float(pData, length, 2)/10.0f;   break;     // 01 01 33 00
        case 2:  bleVal.dataType = MOT_SPEED;       bleVal.fVal = int2float(pData, length, 2)/10.0f;   break;     // 01 02 61 00
        case 4:  bleVal.dataType = MOT_ODOMETER;    bleVal.fVal = int2float(pData, length, 4)/1000.0f; break;     // 01 04 9e d1 39 00
        case 5:  bleVal.dataType = MOT_ASSISTLEVEL; bleVal.fVal = int2float(pData, length, 2);         break;     // 01 05 02 00
        case 7:  bleVal.dataType = MOT_TEMP;        bleVal.fVal = int2float(pData, length, 1);         break;     // 01 07 19
        case 12: bleVal.dataType = MOT_POWER;       bleVal.fVal = int2float(pData, length, 2)/10.0f;   break;     // 01 0c 02 00
        case 16: bleVal.dataType = MOT_PEAKASSIST;  bleVal.unionType = BINARY; bleVal.raw.len = 3; memcpy( bleVal.raw.data, &pData[2], 3 ); break;  // 01 10 <max1> <max2> <max3> 32
        case 21: bleVal.dataType = MOT_SHUTTLE;     bleVal.fVal = int2float(pData, length, 1);         break;     // 01 15 00 
        }
        break;
    case 2: // bike settings
        switch (channel)
        {
        case 0: bleVal.dataType = BIKE_WHEELCIRC;   bleVal.fVal = int2float(pData, length, 2);         break;     // 02 00 fc 08
        case 3: bleVal.dataType = BIKE_ASSISTLEV1;  bleVal.fVal = int2float(pData, length, 1);         break;     // 02 03 0a (32)
        case 4: bleVal.dataType = BIKE_ASSISTLEV2;  bleVal.fVal = int2float(pData, length, 1);         break;     // 02 04 14 (32)
        case 5: bleVal.dataType = BIKE_ASSISTLEV3;  bleVal.fVal = int2float(pData, length, 1);         break;     // 02 05 32 (32)
        case 6: bleVal.dataType = BIKE_FAKECHANNEL; bleVal.fVal = int2float(pData, length, 1);         break;     // 02 06 00 -> bit coded
        case 7: bleVal.dataType = BIKE_ACCEL;       bleVal.fVal = (int2float(pData, length, 2) - 3000.0)/60.0; break;     // 02 07 a0 0f (3000-9000)
        }
        break;
    case 3: // ???
    case 4: // ???
    default:
        break;
    }

    // return unknown message as raw data
    if (bleVal.dataType == LevoEsp32Ble::UNKNOWN)
    {
        bleVal.unionType = BINARY;
        bleVal.raw.len   = length;
        size_t minLen    = min(length, sizeof(bleVal.raw.data));
        memcpy(bleVal.raw.data, pData, minLen);
    }

    return true;
}

// Reconnect client
void LevoEsp32Ble::Reconnect()
{
    Serial.println( "Reconnect called" );
    m_bleStatus = OFFLINE;
    m_bAutoReconnect = true;
    if (s_advDevice == NULL)
    {
        Serial.println("Start scan");
        startScan();
    }
    else
        NimBLEDevice::getScan()->start(LevoEsp32Ble::m_scanTime, scanEndedCB);
}

// Disconnect client
void LevoEsp32Ble::Disconnect()
{
    if( s_advDevice )
    {
        NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(s_advDevice->getAddress());
        if (pClient)
        {
            pClient->disconnect();
            Serial.println("Ble Disconnect() called.");
        }
    }
    m_bAutoReconnect = false;
}

// subscribe to Levo notifications
bool LevoEsp32Ble::Subscribe()
{
    if (s_advDevice)
    {
        Serial.println("Ble Subscribe() called.");
        return subscribe(NimBLEDevice::getClientByPeerAddress(s_advDevice->getAddress()));
    }
    return false;
}

// unsubscribe to Levo notifications
bool LevoEsp32Ble::Unsubscribe()
{
    if (s_advDevice)
    {
        Serial.println("Ble Unsubscribe() called.");
        return unsubscribe(NimBLEDevice::getClientByPeerAddress(s_advDevice->getAddress()));
    }
    return false;
}

// request a particular message
void LevoEsp32Ble::RequestBleValue(enLevoBleDataType valueType)
{
    uint8_t buf[ 2 ];
    buf[0] = ((uint16_t)valueType) >> 8; 
    buf[1] = valueType;
    requestData(buf, 2);
}

// read requested value from levo and compare if message type is as requested
bool LevoEsp32Ble::ReadRequestedBleValue(enLevoBleDataType valueType, stBleVal& bleVal)
{
    uint8_t buf[20];
    size_t length;
    if ((length = readData(buf, sizeof(buf))) > 0 )
    {
        if ( (buf[0] == (((uint16_t)valueType) >> 8)) && (buf[1] == (uint8_t)valueType) )
            return DecodeMessage(buf, length, bleVal);
    }
    return false;
}

// get characteristic of a particular service/characteristic of a connected device
NimBLERemoteCharacteristic* LevoEsp32Ble::getCharacteristic(const char* svcUUIDString, const char* chrUUIDString)
{
    if (s_advDevice)
    {
        NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(s_advDevice->getAddress());
        if (pClient)
        {
            NimBLERemoteService* pSvc = nullptr;
            NimBLERemoteCharacteristic* pChr = nullptr;
            if ((pSvc = pClient->getService(svcUUIDString)) != nullptr)
                return pSvc->getCharacteristic(chrUUIDString);
        }
    }
    return nullptr;
}

// request levo data. Specify the first two bytes of the message (category/channel)
void LevoEsp32Ble::requestData(const uint8_t* pData, size_t length)
{
    NimBLERemoteCharacteristic* pChr = getCharacteristic(LEVO_REQUEST_SERVICE_UUID_STRING, LEVO_REQWRITE_CHAR_UUID_STRING);
    if( pChr && pChr->canWrite() )
        pChr->writeValue( pData, length, true );
}

// read last requested levo data
size_t LevoEsp32Ble::readData(uint8_t* pData, size_t length)
{
    NimBLERemoteCharacteristic* pChr = getCharacteristic(LEVO_REQUEST_SERVICE_UUID_STRING, LEVO_REQREAD_CHAR_UUID_STRING);
    if (pChr && pChr->canRead() )
    {
        std::string val = pChr->readValue();
        if (val.length() > 0)
        {
            int realLength = min(val.length(), length);
            memcpy(pData, val.data(), realLength );
            return realLength;
        }
    }
    return 0;
}

// write levo data
void LevoEsp32Ble::writeData(const uint8_t* pData, size_t length)
{
    NimBLERemoteCharacteristic* pChr = getCharacteristic(LEVO_WRITE_SERVICE_UUID_STRING, LEVO_WRITE_CHAR_UUID_STRING);
    if (pChr && pChr->canWrite() )
       pChr->writeValue(pData, length, true);
}

// subscribe for Levo notications
bool LevoEsp32Ble::subscribe(NimBLEClient* pClient)
{
    bool ret = false;

    if (pClient != nullptr)
    {
        NimBLERemoteService* pSvc = nullptr;
        NimBLERemoteCharacteristic* pChr = nullptr;

        if ((pSvc = pClient->getService(LEVO_DATA_SERVICE_UUID_STRING)) != nullptr)
        {
            if ((pChr = pSvc->getCharacteristic(LEVO_DATA_CHAR_UUID_STRING)) != nullptr)
            {
                if (pChr->canRead())
                {
                    // make an explicit "read" to start authentication
                    pChr->readValue();
                }

                if (pChr->canNotify())
                {
                    if (!pChr->subscribe(true, notifyCB))
                    {
                        // Disconnect if subscribe failed
                        Serial.println("LEVO notification subscription failed");
                        pClient->disconnect();
                        return false;
                    }
                    Serial.println("Subscribed to LEVO notifications");
                    m_bSubscribed = true;
                    ret = true;
                }
            }
        }
    }
    return ret;
}

bool LevoEsp32Ble::unsubscribe(NimBLEClient* pClient)
{
    bool ret = false;

    if( pClient != nullptr)
    {
        NimBLERemoteService* pSvc = nullptr;
        NimBLERemoteCharacteristic* pChr = nullptr;

        if ((pSvc = pClient->getService(LEVO_DATA_SERVICE_UUID_STRING)) != nullptr)
        {
            if ((pChr = pSvc->getCharacteristic(LEVO_DATA_CHAR_UUID_STRING)) != nullptr)
            {
                if (pChr->canNotify())
                {
                    if (!pChr->unsubscribe())
                    {
                        // Disconnect if unsubscribe failed
                        Serial.println("LEVO notification unsubscribe() failed");
                        pClient->disconnect();
                        return false;
                    }
                    Serial.println("Unsubscribed to LEVO notifications");
                    m_bSubscribed = false;
                    ret = true;
                }
            }
        }
    }
    return ret;
}

// Handles the provisioning of clients and connects / interfaces with the server
bool LevoEsp32Ble::connectToServer()
{
    NimBLEClient* pClient = nullptr;
    
    // Check if we have a client we should reuse first
    if( NimBLEDevice::getClientListSize() )
    {
        /** Special case when we already know this device, we send false as the 
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(s_advDevice->getAddress());
        if(pClient)
        {
            if(!pClient->connect(s_advDevice, false))
            {
                Serial.println("Reconnect failed");
                return false;
            }
            Serial.println("Reconnected client");
        } 
        /** We don't already have a client that knows this device,
         *  we will check for a client that is disconnected that we can use.
         */
        else
        {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }
    
    // No client to reuse? Create a new one
    if(!pClient)
    {
        if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
        {
            Serial.println("Max clients reached - no more connections available");
            return false;
        }
        
        pClient = NimBLEDevice::createClient();
        
        Serial.println("New client created");
    
        pClient->setClientCallbacks(&clientCB, false);
        pClient->setConnectionParams(12,12,0,51);
        pClient->setConnectTimeout(5);

        if (!pClient->connect(s_advDevice))
        {
            // Created a client but failed to connect, don't need to keep it as it has no data
            NimBLEDevice::deleteClient(pClient);
            Serial.println("Failed to connect, deleted client");
            return false;
        }
    }         
    
    if(!pClient->isConnected())
    {
        if (!pClient->connect(s_advDevice)) {
            Serial.println("Failed to connect");
            return false;
        }
    }
    
    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());
    
    // Now we can read/write/subscribe the characteristics of the services we are interested in
    subscribe( pClient );

    return true;
}

// get Bluetooth status 
LevoEsp32Ble::enBleStatus LevoEsp32Ble::GetBleStatus()
{
    // manually disabled
    if( m_bleStatus == OFFLINE && !m_bAutoReconnect)
        return SWITCHEDOFF;

    return m_bleStatus;
}

// start scan and connect after finding the Levo device
void LevoEsp32Ble::startScan()
{
    // create new scan
    NimBLEScan* pScan = NimBLEDevice::getScan();

    // create a callback that gets called when advertisers are found
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

    // Set scan interval (how often) and window (how long) in milliseconds
    pScan->setInterval(45);
    pScan->setWindow(15);

    // Active scan will gather scan response data from advertisers but will use more energy from both devices
    pScan->setActiveScan(true);
    // Start scanning for advertisers for the scan time specified (in seconds) 0 = forever, optional callback for when scanning stops. 
    pScan->start(LevoEsp32Ble::m_scanTime, scanEndedCB);
}

void LevoEsp32Ble::Init( uint32_t pin, bool bBtEnabled )
{
    Serial.println("Starting NimBLE Client");

    m_bleStatus = bBtEnabled ? LevoEsp32Ble::OFFLINE : LevoEsp32Ble::SWITCHEDOFF;

    // message queue
    m_bleMsgQueue = xQueueCreate(10, sizeof(stBleMessage));

    // no connection w/o pin
    if (pin == 0)
        return;

    m_pin = pin;

    // Initialize NimBLE, no device name specified as we are not advertising
    NimBLEDevice::init("");
    
    // Set the IO capabilities of the device
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_DISPLAY); // we can enter the PIN via keybord and cann see it on screen
  
    // no bonding (not yet implemented in NimBLE), man in the middle protection, secure connections.
    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND |*/ BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);

    // Optional: set the transmit power, default is 3db
    // NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
    
    // Optional: set any devices you don't want to get advertisments from 
    // NimBLEDevice::addIgnored(NimBLEAddress ("aa:bb:cc:dd:ee:ff")); 
 
    if( bBtEnabled )
    {
        m_bAutoReconnect = true;
        startScan();
    }
}

bool LevoEsp32Ble::Update( stBleVal& bleVal )
{
    bool ret = false;

    // check for BLE values from queue
    stBleMessage bleMsg;
    if (xQueueReceive(m_bleMsgQueue, &bleMsg, 0) == pdPASS)
    {
        // convert raw message to float value
        if (DecodeMessage(bleMsg.data, bleMsg.len, bleVal ) )
            ret = true;
    }

    // Loop here until we find a device we want to connect to
    if (!m_doConnect)
        return ret;
    
    m_doConnect = false;
    
    // Found a device we want to connect to, do it now
    if(connectToServer())
        Serial.println("Success! we should now be getting notifications!");
    else
        Serial.println("Failed to connect, starting scan");

    return ret;
}
