/*
 * 
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 * main entry point for display of all data values on M5
 *
 */

#ifndef M5_SCREEN_H
#define M5_SCREEN_H

#include <M5Core2.h>
#include <Preferences.h>
#include "DisplayData.h"
#include "M5Field.h"
#include "SystemStatus.h"

class M5Screen : public DisplayData
{
public:
    typedef enum
    {
        UNDEFINED = -1,
        SCREEN_A = 0,
        SCREEN_B,
        SCREEN_C,
        NUM_SCREENS,
    } enScreens;

protected:
    const int START_Y = 32;  // Y start position on screen

    typedef struct
    {
        int x;
        int y;
        M5Field* pField;
    } stRender;
    stRender fldRender[numElements];

    int idToIdx[numElements]; // lookup table for fast access from id to idx

    void RenderEmptyValue(stRender& render, const stDisplayData* pDesc );

    // counter for showing system Status to slow down refresh rate
    uint32_t  showSysStatusCnt = 0L;

    // real time clock
    RTC_TimeTypeDef RTCtime_Now;

    // value buffer for ble values in display
    const float FLOAT_UNDEFINED = -10000.0f;
    float valueBuffer[ numElements ]; // index is value id

    // last BLE status 
    LevoEsp32Ble::enBleStatus lastBleStatus = LevoEsp32Ble::UNDEFINED;
    void DrawBluetoothIcon( LevoEsp32Ble::enBleStatus bleStatus );

    // field order on screens, unused elements must be UNKNOWN
    enIds GetIdFromIdx(enScreens nScreen, int i);
    const enIds order[NUM_SCREENS][numElements] =
    {
        {   // Screen A
            BLE_MOT_SPEED,
            BLE_RIDER_POWER,
            BLE_BATT_CURRENT,
            BLE_BATT_VOLTAGE,
            BLE_BATT_REMAINWH,
            BLE_BATT_CHARGEPERCENT,
            BLE_MOT_POWER,
            BLE_MOT_CADENCE,
            BLE_MOT_ASSISTLEVEL,
            BLE_MOT_ODOMETER,
            BLE_BATT_TEMP,
            BLE_MOT_TEMP,
            BLE_BIKE_WHEELCIRC,
            BLE_BATT_CHARGECYCLES,
            BLE_BATT_HEALTH,
            BARO_ALTIMETER, // replace with UNKNOWN to make unvisible
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
        },
        {   // Screen B
            BLE_BATT_SIZEWH,
            BLE_MOT_PEAKASSIST1,
            BLE_MOT_PEAKASSIST2,
            BLE_MOT_PEAKASSIST3,
            BLE_MOT_SHUTTLE,
            BLE_BIKE_ASSISTLEV1,
            BLE_BIKE_ASSISTLEV2,
            BLE_BIKE_ASSISTLEV3,
            BLE_BIKE_FAKECHANNEL,
            BLE_BIKE_ACCEL,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN,
        },
        {   // Screen C
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN,
        }
    };

public:
    M5Screen() { ResetValueBuffer(); }

    void Init(enScreens nScreen, DisplayData& dispData );
    void ShowValue( enIds id, float val, DisplayData& dispData );
    bool ShowSysStatus(SystemStatus& sysStatus);
    void ShowConfig(Preferences& prefs);
    void ResetValueBuffer() { for( int i = 0; i < numElements; i++ ) valueBuffer[i] = FLOAT_UNDEFINED; }
    void UpdateButtonBar(enScreens nScreen);
};

#endif // M5_SCREEN_H