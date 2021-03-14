/** Levo BLE ESP32 Data Logger on M5Core2
 
 *      Author: Bernd Wokoeck
 *
 *  Displays Specialized Levo 2019+ telemetry values to M5Core2 display
 *  and logs data to SD card (levolog.txt)
 *  
 *  Library dependencies:
 *  https://github.com/h2zero/NimBLE-Arduino
 *  https://github.com/m5stack/M5Core2
 *  https://github.com/ropg/ezTime
 *  https://github.com/tanakamasayuki/I2C_BM8563
 *  https://github.com/adafruit/Adafruit_BMP280_Library
 * 
 *  Images converted with: https://lvgl.io/tools/imageconverter (True color, C-Array)
 *
 *      History:
 *      -------------------------------------------------------------
 *      0.90   01/01/2021  created, example for M5Core2 hardware 
 *      0.91   04/02/2021  added: config forms, backlight dim
 *      0.92   07/02/2021  csv log formats added
 *      0.93   13/02/2021  more BLE functionality/data fields added
 *      0.94   16/02/2021  read/write functionality and "tune" dialog
 *      0.95   20/02/2021  small bug fixes, altimeter support (BMP280)
 *      0.96   24/02/2021  virtual sensors (inclination) and trip/avg/peak values
 *                         mot power, batt voltage and batt current corrected
 *                         in LevoEsp32Ble
 *      0.97   12/03/2021  bug fixes (inclination, trip power etc. ), refactoring
 *                         simulator added (see #define): copy CSV full table log
 *                         on SD, delete all non table data and rename it to "simulator.txt"
 *      0.98   14/03/2021  trip range added, bug fixes
 */

#include <M5core2.h>
#include <Preferences.h>
#include <LevoReadWrite.h>
#include "DisplayData.h"
#include "SystemStatus.h"
#include "M5System.h"
#include "M5Screen.h"
#include "FileLogger.h"
#include "M5ConfigFormTune.h"
#include "AltimeterBMP280.h"
#include "VirtualSensors.h"

// #define SIMULATOR
#ifdef SIMULATOR
    #include "Simulator.h"
    Simulator SensorSimulator;
#endif

Preferences     Prefs;
LevoReadWrite   LevoBle;
DisplayData     DispData;
M5System        Core2;
SystemStatus    SysStatus;
M5Screen        Screen( SysStatus );
FileLogger      Logger;
AltimeterBMP280 Altimeter;
VirtualSensors  VirtSensors;

// local settings
FileLogger::enLogFormat _logFormat = FileLogger::CSV_SIMPLE;
bool     _bBtEnabled = true;
uint16_t _backlightTimeout = 60;
bool     _bBacklightCharging = true;
bool     _bHasAltimeter = false;
float    _sealevelhPa = 1013.25;

M5Screen::enScreens currentScreen = M5Screen::SCREEN_A;

// settings button
Button btSettings(220, 0, 100, 60); // top right corner

void installButtonHandlers()
{
    btSettings.addHandler(onBtSettings, E_TOUCH /*E_TAP*/);
    M5.BtnA.addHandler(onBtScreen, E_TOUCH /*E_TAP*/);
    M5.BtnB.addHandler(onBtScreen, E_TOUCH /*E_TAP*/);
    M5.BtnC.addHandler(onBtScreen, E_TOUCH /*E_TAP*/);
    M5.Buttons.draw();
}

void uninstallButtonHandlers()
{
    M5.BtnA.delHandlers();
    M5.BtnB.delHandlers();
    M5.BtnC.delHandlers();
    btSettings.delHandlers();
    Screen.DisableButtonBar();
    M5.Buttons.draw();
}

// read all settings to local settings
void readPreferences()
{
    _logFormat          = (FileLogger::enLogFormat)Prefs.getUChar("LogFormat");
    _bBtEnabled         = Prefs.getUChar("BtEnabled", 1) ? true : false;
    _backlightTimeout   = Prefs.getUShort("BacklightTo", 60);
    _bBacklightCharging = Prefs.getUChar("BacklightChg", 1) ? true : false;
    _sealevelhPa        = Prefs.getFloat("sealevelhPa", 1013.25);

    Serial.printf("Log format: %d\r\n", _logFormat);
    Serial.printf("Bluetooth enabled: %d\r\n", _bBtEnabled);
    Serial.printf("Backlight timeout: %d\r\n", _backlightTimeout);
    Serial.printf("Backlight while charging: %d\r\n", _bBacklightCharging);
    Serial.printf("SealevelPressure (hPa): %f\r\n", _sealevelhPa);
}

// Read bluetooth pin from preferences
uint32_t ReadBluetoothPin()
{
    uint32_t pin = Prefs.getULong("BtPin", 0);
    SysStatus.bHasBtPin = (pin != 0) ? true : false;
    return pin;
}

void onBtSettings(Event& e)
{
    Serial.println("Show settings");
    LevoBle.Disconnect();
    uninstallButtonHandlers();
    // Show menu
    Screen.ShowConfig(Prefs);
    // refresh settings
    readPreferences();
    // reinit display
    Core2.ResetDisplayTimer();
    Core2.SetBacklightSettings( _backlightTimeout, _bBacklightCharging );
    installButtonHandlers();
    Screen.Init(currentScreen, DispData);
    SysStatus.ResetBleStatus();
    Screen.ShowSysStatus();
    // reconnect BLE
    if( _bBtEnabled )
        LevoBle.Reconnect();
}

void onBtTripOrTune(Event& e)
{
    // start: 0, stop: 1, finish: 2, tune: 3
    // tune
    if( e.button->userData == M5Screen::BTTUNE )
    {
        uninstallButtonHandlers();
        M5ConfigFormTune form(LevoBle);
        Core2.ResetDisplayTimer();
        Screen.Init(currentScreen, DispData);
        installButtonHandlers();
    }
    // finish
    else if (e.button->userData == M5Screen::BTFINISH)
    {
        uninstallButtonHandlers();
        if (M5ConfigForms::MsgBox("Finish tour & reset trip data?", M5ConfigForms::YESNO) == M5ConfigForms::RET_YES)
        {
            if( SysStatus.tripStatus != SystemStatus::NONE )
                    VirtSensors.WriteStatisticsSD(DispData);
            VirtSensors.ResetTrip();
            SysStatus.tripStatus = SystemStatus::NONE;
        }
        Core2.ResetDisplayTimer();
        Screen.Init(currentScreen, DispData);
        installButtonHandlers();
    }
    // start
    else if (e.button->userData == M5Screen::BTSTART)
    {
        VirtSensors.StartTrip(DispData);
        SysStatus.tripStatus = SystemStatus::STARTED;
    }
    // stop
    else if (e.button->userData == M5Screen::BTSTOP)
    {
        if( SysStatus.tripStatus == SystemStatus::STARTED )
        {
            VirtSensors.StopTrip();
            SysStatus.tripStatus = SystemStatus::STOPPED;
        }
    }
}

void onBtScreen(Event& e)
{
    M5Screen::enScreens newScreen = M5Screen::UNDEFINED;

    // change screen
    if (M5.BtnA.wasPressed())
        newScreen = M5Screen::SCREEN_A;
    else if (M5.BtnB.wasPressed())
        newScreen = M5Screen::SCREEN_B;
    else if (M5.BtnC.wasPressed())
        newScreen = M5Screen::SCREEN_C;

    if (newScreen != M5Screen::UNDEFINED && newScreen != currentScreen)
    {
        currentScreen = newScreen;
        Screen.Init(currentScreen, DispData);
    }
}

// close log file on BLE disconnect
void BleStatusChanged()
{
    if (LevoBle.GetBleStatus() == LevoEsp32Ble::CONNECTED)
    {
        // open log file
        SysStatus.bLogging = Logger.Open();
    }
    else
    {
        // close log file
        Logger.Close();
        SysStatus.bLogging = false;
    }
}

void ShowBleMaxSupport(LevoEsp32Ble::stBleVal& bleVal, uint32_t timestamp )
{
    // to screen
    Screen.ShowValue(DisplayData::BLE_MOT_PEAKASSIST1, (float)bleVal.raw.data[0], DispData);
    Screen.ShowValue(DisplayData::BLE_MOT_PEAKASSIST2, (float)bleVal.raw.data[1], DispData);
    Screen.ShowValue(DisplayData::BLE_MOT_PEAKASSIST3, (float)bleVal.raw.data[2], DispData);

    // to log file
    LevoEsp32Ble::stBleVal peakAssistVal;
    peakAssistVal.dataType  = LevoEsp32Ble::MOT_PEAKASSIST;
    peakAssistVal.unionType = LevoEsp32Ble::FLOAT;
    peakAssistVal.fVal = (float)bleVal.raw.data[0];
    Logger.Writeln(DisplayData::BLE_MOT_PEAKASSIST1, peakAssistVal, DispData, _logFormat, timestamp );
    peakAssistVal.fVal = (float)bleVal.raw.data[1];
    Logger.Writeln(DisplayData::BLE_MOT_PEAKASSIST2, peakAssistVal, DispData, _logFormat, timestamp );
    peakAssistVal.fVal = (float)bleVal.raw.data[2];
    Logger.Writeln(DisplayData::BLE_MOT_PEAKASSIST3, peakAssistVal, DispData, _logFormat, timestamp );
}

// print and log received bluetooth data
void ShowBleData( LevoEsp32Ble::stBleVal & bleVal, uint32_t timestamp )
{
    DisplayData::enIds id = DisplayData::UNKNOWN;

    // Ble data type to internal display id
    switch (bleVal.dataType)
    {
    case LevoEsp32Ble::BATT_REMAINWH:      id = DisplayData::BLE_BATT_REMAINWH;      break;
    case LevoEsp32Ble::BATT_HEALTH:        id = DisplayData::BLE_BATT_HEALTH;        break;
    case LevoEsp32Ble::BATT_TEMP:          id = DisplayData::BLE_BATT_TEMP;          break;
    case LevoEsp32Ble::BATT_CHARGECYCLES:  id = DisplayData::BLE_BATT_CHARGECYCLES;  break;
    case LevoEsp32Ble::BATT_VOLTAGE:       id = DisplayData::BLE_BATT_VOLTAGE;       break;
    case LevoEsp32Ble::BATT_CURRENT:       id = DisplayData::BLE_BATT_CURRENT;       break;
    case LevoEsp32Ble::BATT_CHARGEPERCENT: id = DisplayData::BLE_BATT_CHARGEPERCENT; break;
    case LevoEsp32Ble::RIDER_POWER:        id = DisplayData::BLE_RIDER_POWER;        break;
    case LevoEsp32Ble::MOT_CADENCE:        id = DisplayData::BLE_MOT_CADENCE;        break;
    case LevoEsp32Ble::MOT_SPEED:          id = DisplayData::BLE_MOT_SPEED;          break;
    case LevoEsp32Ble::MOT_ODOMETER:       id = DisplayData::BLE_MOT_ODOMETER;       break;
    case LevoEsp32Ble::MOT_ASSISTLEVEL:    id = DisplayData::BLE_MOT_ASSISTLEVEL;    break;
    case LevoEsp32Ble::MOT_TEMP:           id = DisplayData::BLE_MOT_TEMP;           break;
    case LevoEsp32Ble::MOT_POWER:          id = DisplayData::BLE_MOT_POWER;          break;
    case LevoEsp32Ble::BIKE_WHEELCIRC:     id = DisplayData::BLE_BIKE_WHEELCIRC;     break;
    case LevoEsp32Ble::BATT_SIZEWH:        id = DisplayData::BLE_BATT_SIZEWH;        break;
    case LevoEsp32Ble::MOT_SHUTTLE:        id = DisplayData::BLE_MOT_SHUTTLE;        break;
    case LevoEsp32Ble::BIKE_ASSISTLEV1:    id = DisplayData::BLE_BIKE_ASSISTLEV1;    break;
    case LevoEsp32Ble::BIKE_ASSISTLEV2:    id = DisplayData::BLE_BIKE_ASSISTLEV2;    break;
    case LevoEsp32Ble::BIKE_ASSISTLEV3:    id = DisplayData::BLE_BIKE_ASSISTLEV3;    break;
    case LevoEsp32Ble::BIKE_FAKECHANNEL:   id = DisplayData::BLE_BIKE_FAKECHANNEL;   break;
    case LevoEsp32Ble::BIKE_ACCEL:         id = DisplayData::BLE_BIKE_ACCEL;         break;
    // motor max support settings (3 values in one message)
    case LevoEsp32Ble::MOT_PEAKASSIST:     ShowBleMaxSupport(bleVal, timestamp);     return;
    }

    // display and log decoded value
    if( id != DisplayData::UNKNOWN && bleVal.unionType == LevoEsp32Ble::FLOAT )
    {
        ShowFloatData(id, bleVal.fVal, timestamp );
        VirtSensors.FeedValue(id, bleVal.fVal, timestamp);  // notify virtual sensors
    }
    else
    {
        // log unknown data and dump to serial
        Logger.Writeln(id, bleVal, DispData, _logFormat, timestamp );
    }
}

// print and log float values from non BLE sensors
void ShowFloatData(DisplayData::enIds id, float fVal, uint32_t timestamp )
{
    Screen.ShowValue(id, fVal, DispData); // ouput to screen
    // log data and dump to serial
    LevoEsp32Ble::stBleVal bleVal;
    bleVal.unionType = LevoEsp32Ble::FLOAT; bleVal.fVal = fVal;
    Logger.Writeln(id, bleVal, DispData, _logFormat, timestamp );
}

// sensor simulator
#ifdef SIMULATOR
void Simulate(uint32_t timestamp)
{
    DisplayData::enIds id;
    float fVal;
    // _bBtEnabled = true;
    // SysStatus.UpdateBleStatus(LevoEsp32Ble::CONNECTED);
    _bHasAltimeter = false;
    if (SensorSimulator.Update( id, fVal, timestamp ) )
    {
        // Serial.printf("id: %d, val: %f\r\n", id, fVal );
        Screen.ShowValue(id, fVal, DispData); // ouput to screen
        VirtSensors.FeedValue(id, fVal, timestamp);
    }
}
#endif

void setup ()
{
    Prefs.begin("LevoEsp32", false);

    // M5Core2 system init
    Core2.Init();
    Core2.CheckSDCard(SysStatus);
    delay(500);

    // get settings
    readPreferences();
    Core2.SetBacklightSettings(_backlightTimeout, _bBacklightCharging);

    // altimeter
    _bHasAltimeter = Altimeter.Init();
    if( !_bHasAltimeter )
    {
        DispData.Hide(DisplayData::BARO_ALTIMETER );
        DispData.Hide(DisplayData::VIRT_INCLINATION);
        DispData.Hide(DisplayData::TRIP_ELEVATIONGAIN);
    }

    // all screen output
    Screen.SetButtonBarHandler( onBtTripOrTune );
    Screen.Init(M5Screen::SCREEN_A, DispData);

    // bluetooth communication
    LevoBle.Init( ReadBluetoothPin(), _bBtEnabled );

    // buttons
    installButtonHandlers();
}

// running on core 1: xPortGetCoreID()
//////////////////////////////////////
void loop ()
{
    static uint32_t ti50   = millis() + 50L;
    static uint32_t ti100  = millis() + 100L;
    static uint32_t ti1000 = millis() + 1000L;

    // call timer once
    uint32_t ti = millis();

    // bluetooth handling
    LevoEsp32Ble::stBleVal bleVal;
    if (LevoBle.Update(bleVal))
        ShowBleData( bleVal, ti );

    #ifdef SIMULATOR
        Simulate( ti );
    #endif 

    // do all 50ms
    if (ti >= ti50)
    {
        ti50 = ti + 50L;
        // get virtual sensor values
        DisplayData::enIds id; float fVal;
        if (VirtSensors.Update(id, fVal, ti))
            ShowFloatData(id, fVal, ti);
    }

    // do all 100ms
    if (ti >= ti100)
    {
        ti100 = ti + 100L;
        // print system status to screen
        SysStatus.UpdateBleStatus( LevoBle.GetBleStatus() );
        Core2.CheckPower(SysStatus);
        if (Screen.ShowSysStatus())
            BleStatusChanged();

        // dim display after some time
        Core2.DoDisplayTimer();
    }

    // do all 1000ms
    if (ti >= ti1000)
    {
        ti1000 = ti + 1000L;
        // Altimeter
        if (_bHasAltimeter)
        {
            float altitude = Altimeter.GetAltitude(_sealevelhPa);
            ShowFloatData( DisplayData::BARO_ALTIMETER, altitude, ti );
            VirtSensors.FeedValue(DisplayData::BARO_ALTIMETER, altitude, ti);  // notify virtual sensors
        }
    }

    // touch update
    M5.update();
}
