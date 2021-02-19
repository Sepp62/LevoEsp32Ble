/** Levo ESP32 BLE Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 *  Derived from NimBLE client sample: H2zero: https://github.com/h2zero/NimBLE-Arduino
 */

#ifndef LEVOESP32BLE_H
#define LEVOESP32BLE_H

#include "Arduino.h"
#include <NimBLEDevice.h>

class NimBLERemoteCharacteristic;

//---------------
// Levo data notifications
#define LEVO_DATA_SERVICE_UUID_STRING "00000003-3731-3032-494d-484f42525554"
#define LEVO_DATA_CHAR_UUID_STRING "00000013-3731-3032-494d-484f42525554"

// Levo data request & read
#define LEVO_REQUEST_SERVICE_UUID_STRING "00000001-3731-3032-494d-484f42525554"
#define LEVO_REQWRITE_CHAR_UUID_STRING "00000021-3731-3032-494d-484f42525554"   // ping pong: first request a part. value via writing a request...
#define LEVO_REQREAD_CHAR_UUID_STRING "00000011-3731-3032-494d-484f42525554"    // ...then read the value back via parameterless read request

// Levo data write
#define LEVO_WRITE_SERVICE_UUID_STRING "00000002-3731-3032-494d-484f42525554"
#define LEVO_WRITE_CHAR_UUID_STRING "00000012-3731-3032-494d-484f42525554"

class LevoEsp32Ble
{
public:
    typedef enum
    {
        UNKNOWN            = -1,
        
        // 16 bit value corresponds with first 2 bytes of a message
        BATT_SIZEWH        = 0x0000 | 0x0000,
        BATT_REMAINWH      = 0x0000 | 0x0001,
        BATT_HEALTH        = 0x0000 | 0x0002,
        BATT_TEMP          = 0x0000 | 0x0003,
        BATT_CHARGECYCLES  = 0x0000 | 0x0004,
        BATT_VOLTAGE       = 0x0000 | 0x0005, // unclear
        BATT_CURRENT       = 0x0000 | 0x0006, // unclear
        BATT_CHARGEPERCENT = 0x0000 | 0x000c,

        RIDER_POWER        = 0x0100 | 0x0000,
        MOT_CADENCE        = 0x0100 | 0x0001,
        MOT_SPEED          = 0x0100 | 0x0002,
        MOT_ODOMETER       = 0x0100 | 0x0004,
        MOT_ASSISTLEVEL    = 0x0100 | 0x0005,
        MOT_TEMP           = 0x0100 | 0x0007,
        MOT_POWER          = 0x0100 | 0x000c,
        MOT_PEAKASSIST     = 0x0100 | 0x0010,  // for assist level 1, 2, 3
        MOT_SHUTTLE        = 0x0100 | 0x0015,

        BIKE_WHEELCIRC     = 0x0200 | 0x0000,
        BIKE_ASSISTLEV1    = 0x0200 | 0x0003,
        BIKE_ASSISTLEV2    = 0x0200 | 0x0004,
        BIKE_ASSISTLEV3    = 0x0200 | 0x0005,
        BIKE_FAKECHANNEL   = 0x0200 | 0x0006,
        BIKE_ACCEL         = 0x0200 | 0x0007,
    } enLevoBleDataType;

    typedef enum
    {
        UNDEFINED = 0,
        SWITCHEDOFF,
        OFFLINE,
        CONNECTING,
        CONNECTED,
        AUTHERROR
    } enBleStatus;

    typedef enum
    {
        FLOAT     =  0,
        BINARY    =  1
    } enUnionType;

    typedef struct stBleVal
    {
        enLevoBleDataType dataType  = UNKNOWN;
        enUnionType       unionType = FLOAT;
        union
        {
            float fVal = 0.0f;
            struct
            {
                size_t  len;
                uint8_t data[20];
            } raw;
        };
    } stBleVal;

protected:
    friend class AdvertisedDeviceCallbacks;
    friend class ClientCallbacks;
    friend void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

    static uint32_t      m_pin;
    static enBleStatus   m_bleStatus;
    static bool          m_doConnect;
    static uint32_t      m_scanTime; /** 0 = scan forever */
    static bool          m_queueOverrun;

    static bool          m_bAutoReconnect;
    static bool          m_bSubscribed;

    // BLE message queue 
    typedef struct
    {
        size_t  len;
        uint8_t data[20]; // no messages longer than 20 bytes observed
    } stBleMessage;
    static QueueHandle_t m_bleMsgQueue;

    // decode ble data message to float values
    float int2float(uint8_t* pData, size_t length, size_t valueSize = -1, int bufOffset = 2 );
    bool  DecodeMessage( uint8_t* pData, size_t length, stBleVal & bleVal );

public:
    LevoEsp32Ble() {}

    void Init( uint32_t pin, bool bBtEnabled = true );
    bool Update( stBleVal & bleVal );
    void Disconnect();
    void Reconnect();
    bool Subscribe();
    bool Unsubscribe();

    enBleStatus GetBleStatus();
    bool        IsConnected() { return (m_bleStatus == CONNECTED); }
    bool        IsSubscribed() { return m_bSubscribed; }

    void RequestBleValue(enLevoBleDataType valueType);
    bool ReadRequestedBleValue(enLevoBleDataType valueType, stBleVal& bleVal);

protected:
    void startScan();
    bool connectToServer();

    // read and write BLE levo data
    NimBLERemoteCharacteristic* getCharacteristic( const char * svcUUIDString, const char* chrUUIDString );
    void   requestData( const uint8_t * pData, size_t length );
    size_t readData(uint8_t * pData, size_t length);
    void   writeData(const uint8_t* pData, size_t length);

    bool subscribe( NimBLEClient* pClient );
    bool unsubscribe( NimBLEClient* pClient );
};
#endif // LEVOESP32BLE_H
