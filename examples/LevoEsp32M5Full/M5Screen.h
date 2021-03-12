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
#include "M5TripTuneButtons.h"

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

    typedef enum // keep consistent with code in M5TripTuneButtons (userData)
    {
        BTSTART = M5TripTuneButtons::BTSTART,
        BTSTOP  = M5TripTuneButtons::BTSTOP, 
        BTRESET = M5TripTuneButtons::BTRESET,
        BTTUNE  = M5TripTuneButtons::BTTUNE,
    } enButtons;

protected:
    const int START_Y = 32;  // Y start position on screen

    typedef struct
    {
        int x;
        int y;
        M5Field* pField;
    } stRender;
    stRender m_fldRender[numElements];

    int m_idToIdx[numElements]; // lookup table for fast access from id to idx (for current screen only)

    void renderEmptyValue(stRender& render, const stDisplayData* pDesc );
    void formatAsTime(float val, size_t nLen, char * strVal);

    // counter for showing system Status to slow down refresh rate
    uint32_t  m_showSysStatusCnt = 0L;

    // real time clock
    RTC_TimeTypeDef m_RTCtime_Now;

    // value buffer for ble values in display
    const float FLOAT_UNDEFINED = -10000.0f;
    float m_valueBuffer[ numElements ]; // index is value id

    // last BLE status 
    LevoEsp32Ble::enBleStatus m_lastBleStatus = LevoEsp32Ble::UNDEFINED;
    void drawBluetoothIcon( LevoEsp32Ble::enBleStatus bleStatus );

    // start/stop/reset and tune
    M5TripTuneButtons m_tripTuneButtons;
    void (*m_fnButtonBarEvent)(Event&) = NULL;
    void enableButtonBar(void (*fnBtEvent)(Event&)) { m_tripTuneButtons.Enable(fnBtEvent); }

    // field order on screens, unused elements must be UNKNOWN
    const uint32_t NEWLINE = 0x80000000;
    bool hasLineBreak(enScreens nScreen, int i);
    enIds getIdFromIdx(enScreens nScreen, int i);
    const uint32_t m_order[NUM_SCREENS][numElements] =
    {
        {   // Screen A
            BLE_MOT_SPEED,
            BLE_RIDER_POWER,
            BLE_MOT_POWER,
            BLE_BATT_CURRENT,
            BLE_BATT_VOLTAGE,
            BLE_BATT_REMAINWH,
            BLE_BATT_CHARGEPERCENT,
            BLE_MOT_CADENCE,
            BLE_MOT_ASSISTLEVEL,
            BLE_MOT_ODOMETER,
            BLE_BATT_TEMP,
            BLE_MOT_TEMP,
            BARO_ALTIMETER,
            VIRT_INCLINATION,
            VIRT_CONSUMPTION,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN,
        },
        {   // Screen B
            BLE_MOT_PEAKASSIST1,
            BLE_MOT_PEAKASSIST2,
            BLE_MOT_PEAKASSIST3,
            BLE_BIKE_ASSISTLEV1 | NEWLINE,
            BLE_BIKE_ASSISTLEV2,
            BLE_BIKE_ASSISTLEV3,
            BLE_BIKE_FAKECHANNEL | NEWLINE,
            BLE_MOT_SHUTTLE,
            BLE_BIKE_ACCEL,
            BLE_BATT_SIZEWH | NEWLINE,
            BLE_BIKE_WHEELCIRC,
            BLE_BATT_CHARGECYCLES,
            BLE_BATT_HEALTH,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
        },
        {   // Screen C
            TRIP_DISTANCE,
            TRIP_TIME,
            TRIP_ELEVATIONGAIN,
            TRIP_AVGSPEED,
            TRIP_MAXSPEED,
            TRIP_MINBATTVOLTAGE,
            TRIP_RIDERENERGY,
            TRIP_BATTENERGY,
            TRIP_PEAKMOTTEMP,
            TRIP_PEAKBATTTEMP,
            TRIP_PEAKBATTCURRENT,
            TRIP_PEAKRIDERPOWER,
            TRIP_PEAKMOTORPOWER,
            TRIP_CONSUMPTION, // TRIP_MOTORENERGY,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
        }
    };

public:
    M5Screen() { ResetValueBuffer(); }

    void Init(enScreens nScreen, DisplayData& dispData );
    void ShowValue( enIds id, float val, DisplayData& dispData );
    bool ShowSysStatus(SystemStatus& sysStatus);
    void ShowConfig(Preferences& prefs);
    void ResetValueBuffer() { for( int i = 0; i < numElements; i++ ) m_valueBuffer[i] = FLOAT_UNDEFINED; }
    void UpdateHardwareButtons(enScreens nScreen);

    void SetButtonBarHandler(void (*fnBtEvent)(Event&)) { m_fnButtonBarEvent = fnBtEvent; }
    void DisableButtonBar() { enableButtonBar( NULL ); }
    void SetButtonBarButtonState(enButtons btId, bool bChecked) { m_tripTuneButtons.SetButtonState((M5TripTuneButtons::enButtons)btId, bChecked); }
};

#endif // M5_SCREEN_H