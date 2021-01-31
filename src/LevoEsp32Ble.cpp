/** Levo ESP32 BLE Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 *  Derived from NimBLE client sample: H2zero: https://github.com/h2zero/NimBLE-Arduino
 */

#include <NimBLEDevice.h>
#include "LevoEsp32Ble.h"

// bluetooth pin
uint32_t LevoEsp32Ble::m_pin = 0L;

// connection status
LevoEsp32Ble::enBleStatus LevoEsp32Ble::m_bleStatus = LevoEsp32Ble::OFFLINE;

// ble connect "state machine
bool     LevoEsp32Ble::m_doConnect = false;
uint32_t LevoEsp32Ble::m_scanTime = 0; /** 0 = scan forever */
bool     LevoEsp32Ble::m_queueOverrun = false;

// ble message queue from bluetooth stack running on core 0 to main thread on core 1
QueueHandle_t LevoEsp32Ble::m_bleMsgQueue;

void scanEndedCB(NimBLEScanResults results);

static NimBLEAdvertisedDevice* advDevice;

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
        NimBLEDevice::getScan()->start(LevoEsp32Ble::m_scanTime, scanEndedCB);
    };
    
    /** Called when the peripheral requests a change to the connection parameters.
     *  Return true to accept and apply them or false to reject and keep 
     *  the currently used parameters. Default will return true.
     */
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params)
    {
        // accept changed timing, "return false" is not liked by Levo
        Serial.print("onConnParamsUpdateRequest(min,max,lat,to):");
        Serial.print(params->itvl_min); Serial.print(", "); // in 1.25ms units
        Serial.print(params->itvl_max); Serial.print(", "); // in 1.25ms units
        Serial.print(params->latency); Serial.print(", ");
        Serial.println(params->supervision_timeout);  // in 10ms
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
                advDevice = advertisedDevice;
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

// decode our BLE message to get Levo data
//////////////////////////////////////////
bool LevoEsp32Ble::DecodeMessage(uint8_t* pData, size_t length, LevoEsp32Ble::stBleVal & bleVal )
{
    bleVal.dataType  = LevoEsp32Ble::UNKNOWN;
    bleVal.unionType = FLOAT;

    uint8_t  sender = pData[0];
    uint8_t  channel = pData[1];
    uint32_t val = 0L;

    // calculate integer value from data length (TODO works for int values only)
    if (length == 3)
        val = pData[2];
    else if (length == 4)
        val = pData[2] + (((uint32_t)(pData[3])) << 8);
    else if (length == 6)
        val = pData[2] + (((uint32_t)(pData[3])) << 8) + (((uint32_t)(pData[4])) << 16) + (((uint32_t)(pData[5])) << 24);

    switch (sender)
    {
    case 0: // battery
        switch (channel)
        {
        case 1:  bleVal.dataType = LevoEsp32Ble::BATT_ENERGY;         bleVal.fVal = val; break; // 00 01 e4 00 Batt Wh? 4Wh per %  -> 51%->228  52%->233 33%->146 33%->144   full=450 or 504 or 400?
        case 2:  bleVal.dataType = LevoEsp32Ble::BATT_HEALTH;         bleVal.fVal = val; break; // 00 02 64
        case 3:  bleVal.dataType = LevoEsp32Ble::BATT_TEMP;           bleVal.fVal = val; break; // 00 03 13
        case 4:  bleVal.dataType = LevoEsp32Ble::BATT_CHARGECYCLES;   bleVal.fVal = val; break; // 00 04 0d 00
        case 5:  bleVal.dataType = LevoEsp32Ble::BATT_VOLTAGE;        bleVal.fVal = 28.0f + ((float)val) / 10.0f; break; // 00 05 50
        case 6:  bleVal.dataType = LevoEsp32Ble::BATT_CURRENT;        bleVal.fVal = ((float)val) / 10.0f; break; // 00 06 00
        case 12: bleVal.dataType = LevoEsp32Ble::BATT_CHARGEPERCENT;  bleVal.fVal = val; break; // 00 0c 34
        }
        break;
    case 1: // motor
        switch (channel)
        {
        case 0:  bleVal.dataType = LevoEsp32Ble::RIDER_POWER;     bleVal.fVal = val; break;                      // 01 00 00 00
        case 1:  bleVal.dataType = LevoEsp32Ble::MOT_CADENCE;     bleVal.fVal = ((float)val) / 10.0f; break;     // 01 01 33 00
        case 2:  bleVal.dataType = LevoEsp32Ble::MOT_SPEED;       bleVal.fVal = ((float)val) / 10.0f; break;     // 01 02 61 00
        case 4:  bleVal.dataType = LevoEsp32Ble::MOT_ODOMETER;    bleVal.fVal = ((float)val) / 1000.0f; break;   // 01 04 9e d1 39 00
        case 5:  bleVal.dataType = LevoEsp32Ble::MOT_ASSISTLEVEL; bleVal.fVal = val; break;                      // 01 05 02 00
        case 7:  bleVal.dataType = LevoEsp32Ble::MOT_TEMP;        bleVal.fVal = val; break;                      // 01 07 19
        case 12: bleVal.dataType = LevoEsp32Ble::MOT_POWER;       bleVal.fVal = ((float)val) / 10.0f; break;     // 01 0c 02 00
        }
        break;
    case 2: // bike settings
        switch (channel)
        {
        case 0:  bleVal.dataType = LevoEsp32Ble::BIKE_WHEELCIRC; bleVal.fVal = val; break;                       // 02 00 fc 08
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
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if(pClient)
        {
            if(!pClient->connect(advDevice, false))
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

        if (!pClient->connect(advDevice))
        {
            // Created a client but failed to connect, don't need to keep it as it has no data
            NimBLEDevice::deleteClient(pClient);
            Serial.println("Failed to connect, deleted client");
            return false;
        }
    }         
    
    if(!pClient->isConnected())
    {
        if (!pClient->connect(advDevice)) {
            Serial.println("Failed to connect");
            return false;
        }
    }
    
    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());
    
    // Now we can read/write/subscribe the charateristics of the services we are interested in
    NimBLERemoteService* pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;
    NimBLERemoteDescriptor* pDsc = nullptr;
    
    if(( pSvc = pClient->getService(LEVO_DATA_SERVICE_UUID_STRING) ) != nullptr )
    {
        if(( pChr = pSvc->getCharacteristic(LEVO_DATA_CHAR_UUID_STRING)) != nullptr )
        {   
            if(pChr->canRead())
            {
                // make an explicit "read" to start authentication
                pChr->readValue();
            }
        
            if(pChr->canNotify())
            {
                if(!pChr->subscribe(true, notifyCB))
                {
                    // Disconnect if subscribe failed
                    pClient->disconnect();
                    return false;
                }
                Serial.println("Subscribed to LEVO notifications");
            }
        }
    }
    else
    {
        Serial.println("LEVO service not found.");
    }
    return true;
}

void LevoEsp32Ble::Init( uint32_t pin )
{
    Serial.println("Starting NimBLE Client");

    m_pin = pin;

    // message queue
    m_bleMsgQueue = xQueueCreate(10, sizeof(stBleMessage));

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

