/*
 * 
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
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

class M5Screen
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
        BTSTART  = M5TripTuneButtons::BTSTART,
        BTSTOP   = M5TripTuneButtons::BTSTOP, 
        BTFINISH = M5TripTuneButtons::BTFINISH,
        BTTUNE   = M5TripTuneButtons::BTTUNE,
    } enButtons;

protected:
    SystemStatus& m_sysStatus;

    const int START_Y = 32;  // Y start position on screen

    typedef struct
    {
        int x = 0;
        int y = 0;
        M5Field* pField = 0;
    } stRender;
    stRender m_fldRender[DisplayData::numElements];

    int m_idToIdx[DisplayData::numElements]; // lookup table for fast access from id to idx (for current screen only)

    void renderEmptyValue(stRender& render, const DisplayData::stDisplayData* pDesc );
    void formatAsTime(float val, size_t nLen, char * strVal);

    // counter for showing system Status to slow down refresh rate
    uint32_t  m_showSysStatusCnt = 0L;

    // real time clock
    RTC_TimeTypeDef m_RTCtime_Now;

    // value buffer for ble values in display
    const float FLOAT_UNDEFINED = -10000.0f;
    float m_valueBuffer[ DisplayData::numElements ]; // index is value id

    // last BLE status 
    LevoEsp32Ble::enBleStatus m_lastBleStatus = LevoEsp32Ble::UNDEFINED;
    void drawBluetoothIcon( LevoEsp32Ble::enBleStatus bleStatus );

    // start/stop/reset and tune
    M5TripTuneButtons m_tripTuneButtons;
    void (*m_fnButtonBarEvent)(Event&) = NULL;
    void enableButtonBar(void (*fnBtEvent)(Event&)) { m_tripTuneButtons.Enable(fnBtEvent); }
    void updateTripButtonStatus();

    // field order on screens, unused elements must be UNKNOWN
    const uint32_t NEWLINE = 0x80000000;
    const uint32_t UNKNOWN = DisplayData::UNKNOWN;
    bool hasLineBreak(enScreens nScreen, int i);
    DisplayData::enIds getIdFromIdx(enScreens nScreen, int i);
    const uint32_t m_order[NUM_SCREENS][DisplayData::numElements] =
    {
        {   // Screen A
            DisplayData::BLE_MOT_SPEED,
            DisplayData::BLE_RIDER_POWER,
            DisplayData::BLE_MOT_POWER,
            DisplayData::BLE_BATT_CURRENT,
            DisplayData::BLE_BATT_VOLTAGE,
            DisplayData::BLE_BATT_REMAINWH,
            DisplayData::BLE_BATT_CHARGEPERCENT,
            DisplayData::BLE_MOT_CADENCE,
            DisplayData::BLE_MOT_ASSISTLEVEL,
            DisplayData::BLE_MOT_ODOMETER,
            DisplayData::BLE_BATT_TEMP,
            DisplayData::BLE_MOT_TEMP,
            DisplayData::BARO_ALTIMETER,
            DisplayData::VIRT_INCLINATION,
            DisplayData::VIRT_CONSUMPTION,
            DisplayData::TRIP_RANGE,
            DisplayData::PWR_POWER,
            UNKNOWN, // DisplayData::BARO_TEMP, // DisplayData::GYRO_PITCH,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
        },
        {   // Screen B
            DisplayData::BLE_MOT_PEAKASSIST1,
            DisplayData::BLE_MOT_PEAKASSIST2,
            DisplayData::BLE_MOT_PEAKASSIST3,
            DisplayData::BLE_BIKE_ASSISTLEV1 | NEWLINE,
            DisplayData::BLE_BIKE_ASSISTLEV2,
            DisplayData::BLE_BIKE_ASSISTLEV3,
            DisplayData::BLE_BIKE_FAKECHANNEL | NEWLINE,
            DisplayData::BLE_MOT_SHUTTLE,
            DisplayData::BLE_BIKE_ACCEL,
            DisplayData::BLE_BATT_SIZEWH | NEWLINE,
            DisplayData::BLE_BIKE_WHEELCIRC,
            DisplayData::BLE_BATT_CHARGECYCLES,
            DisplayData::BLE_BATT_HEALTH,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
        },
        {   // Screen C
            DisplayData::TRIP_DISTANCE,
            DisplayData::TRIP_TIME,
            DisplayData::TRIP_ELEVATIONGAIN,
            DisplayData::TRIP_RIDERPOWER,
            DisplayData::TRIP_AVGSPEED,
            DisplayData::TRIP_MAXSPEED,
            DisplayData::TRIP_MINBATTVOLTAGE,
            DisplayData::TRIP_RIDERENERGY,
            DisplayData::TRIP_BATTENERGY,
            DisplayData::TRIP_PEAKMOTTEMP,
            DisplayData::TRIP_PEAKBATTTEMP,
            DisplayData::TRIP_PEAKBATTCURRENT,
            DisplayData::TRIP_PEAKRIDERPOWER,
            DisplayData::TRIP_PEAKMOTORPOWER,
            DisplayData::TRIP_CONSUMPTION, // TRIP_MOTORENERGY,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
            UNKNOWN, UNKNOWN, UNKNOWN,
        }
    };

public:
    M5Screen(SystemStatus& rSystemStatus) : m_sysStatus(rSystemStatus) { ResetValueBuffer(); }

    void Init(enScreens nScreen, DisplayData& dispData );
    void ShowValue( DisplayData::enIds id, float val, DisplayData& dispData );
    bool ShowSysStatus();
    void ShowConfig(Preferences& prefs);
    void ResetValueBuffer() { for( int i = 0; i < DisplayData::numElements; i++ ) m_valueBuffer[i] = FLOAT_UNDEFINED; }
    void UpdateHardwareButtons(enScreens nScreen);

    void SetButtonBarHandler(void (*fnBtEvent)(Event&)) { m_fnButtonBarEvent = fnBtEvent; }
    void DisableButtonBar() { enableButtonBar( NULL ); }
    void SetButtonBarButtonState(enButtons btId, bool bChecked) { m_tripTuneButtons.SetButtonState((M5TripTuneButtons::enButtons)btId, bChecked); }
};

#endif // M5_SCREEN_H