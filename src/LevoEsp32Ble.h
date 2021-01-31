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

class NimBLERemoteCharacteristic;

//---------------
// The remote service for Levo data
#define LEVO_DATA_SERVICE_UUID_STRING "00000003-3731-3032-494d-484f42525554"
// The characteristic of the remote service that sends data notifications
#define LEVO_DATA_CHAR_UUID_STRING "00000013-3731-3032-494d-484f42525554"

/* More services:
https://www.pedelecforum.de/forum/index.php?threads/bluetooth-schnittstelle-im-akku-von-levo-kenevo.56785/
Service "00000001-3731-3032-494d-484f42525554"
    Char write "00000021-3731-3032-494d-484f42525554"
    Char read  "00000011-3731-3032-494d-484f42525554"

Service "00000002-3731-3032-494d-484f42525554"
    Char write "00000012-3731-3032-494d-484f42525554"
*/

class LevoEsp32Ble
{
public:
    typedef enum
    {
        UNKNOWN            = -1,

        BATT_ENERGY        = 1, // unclear
        BATT_HEALTH        = 2,
        BATT_TEMP          = 3,
        BATT_CHARGECYCLES  = 4,
        BATT_VOLTAGE       = 5, // unclear
        BATT_CURRENT       = 6, // unclear
        BATT_CHARGEPERCENT = 12,

        RIDER_POWER        = 100,
        MOT_CADENCE        = 101,
        MOT_SPEED          = 102,
        MOT_ODOMETER       = 104,
        MOT_ASSISTLEVEL    = 105,
        MOT_TEMP           = 107,
        MOT_POWER          = 112,

        BIKE_WHEELCIRC     = 200,
    } enLevoBleDataType;

    typedef enum
    {
        UNDEFINED = 0,
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

    typedef struct
    {
        enLevoBleDataType dataType  = UNKNOWN;
        enUnionType       unionType = FLOAT;
        union
        {
            float fVal = 0.0f;
            struct
            {
                size_t  len;
                uint8_t data[18];
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

    // BLE message queue 
    typedef struct
    {
        size_t  len;
        uint8_t data[18]; // no messages longer than 18 bytes observed
    } stBleMessage;
    static QueueHandle_t m_bleMsgQueue;

    // decode ble data message to float values
    bool DecodeMessage( uint8_t* pData, size_t length, stBleVal & bleVal );

public:
    LevoEsp32Ble() {}

    void Init( uint32_t pin );
    bool Update( stBleVal & bleVal );

    enBleStatus GetBleStatus() { return m_bleStatus; }

protected:

    bool connectToServer();
};
#endif // LEVOESP32BLE_H
