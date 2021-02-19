/** Levo ESP32 BLE Data Dumper
 *
 *  Created: 15/02/2021
 *      Author: Bernd Woköck
 *
 *  Enhances LevoEsp32Ble with read/write functionality
 */

#ifndef LEVOREADWRITE_H
#define LEVOREADWRITE_H

#include <LevoEsp32Ble.h>

class LevoReadWrite : public LevoEsp32Ble
{
public:

    typedef enum
    {
        INVALID = -1,
        ASSIST_OFF = 0,
        ASSIST_ECO,
        ASSIST_TRAIL,
        ASSIST_TURBO
    } enAssistLevel;

    bool WriteAssistLevel(enAssistLevel enLevel );
    enAssistLevel GetAssistLevel();
    bool ReadSync(enLevoBleDataType valueType, stBleVal& bleVal, uint32_t timeout = 1000L ); // sync read
    bool ReadAsync(enLevoBleDataType valueType, bool bForce = false );                       // async request read, result is delivered via Update()
    bool Update(stBleVal& bleVal);                                                           // periodically called from loop()

    enum
    {   // writeMask
        BIT_PEAKASSIST = 1 << 0,
        BIT_ASSIST = 1 << 1,
        BIT_SHUTTLE = 1 << 2,
        BIT_ACCELSENS = 1 << 3,
        BIT_FAKECHAN = 1 << 4,
        ALL = 0xFFFF,
    };

    typedef struct
    {
        enum { NUM_LEVELS = 3 };
        int8_t assist[NUM_LEVELS];
        int8_t peakAssist[NUM_LEVELS];
        int8_t shuttle;
        int8_t accelSens;
        int8_t fakeChannel;
    } stLevoAssist;
    bool ReadAssistDataFields(stLevoAssist & assistData );
    bool WriteAssistDataFields(stLevoAssist& assistData, uint16_t writeMask = ALL);

protected:
    const enLevoBleDataType m_assistDataFields[7] = 
    {
            MOT_PEAKASSIST, MOT_SHUTTLE,
            BIKE_ASSISTLEV1, BIKE_ASSISTLEV2, BIKE_ASSISTLEV3,
            BIKE_FAKECHANNEL, BIKE_ACCEL
    };

    enLevoBleDataType m_requestedValueType = UNKNOWN;

    void resetAssistData(stLevoAssist& supportData);
    bool isValidAssistData(stLevoAssist& supportData);
};

#endif // LEVOREADWRITE_H