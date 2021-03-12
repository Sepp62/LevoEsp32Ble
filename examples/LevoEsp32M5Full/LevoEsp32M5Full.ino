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

Preferences     Prefs;
LevoReadWrite   LevoBle;
DisplayData     DispData;
M5System        Core2;
M5Screen        Screen;
SystemStatus    SysStatus;
FileLogger      Logger;
AltimeterBMP280 Altimeter;
VirtualSensors  VirtSensors;

// local settings
FileLogger::enLogFormat logFormat = FileLogger::CSV_SIMPLE;
bool     bBtEnabled = true;
uint16_t backlightTimeout = 60;
bool     bBacklightCharging = true;
bool     bHasAltimeter = false;
float    sealevelhPa = 1013.25;

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
    logFormat          = (FileLogger::enLogFormat)Prefs.getUChar("LogFormat");
    bBtEnabled         = Prefs.getUChar("BtEnabled", 1) ? true : false;
    backlightTimeout   = Prefs.getUShort("BacklightTo", 60);
    bBacklightCharging = Prefs.getUChar("BacklightChg", 1) ? true : false;
    sealevelhPa        = Prefs.getFloat("sealevelhPa", 1013.25);

    Serial.printf("Log format: %d\r\n", logFormat);
    Serial.printf("Bluetooth enabled: %d\r\n", bBtEnabled);
    Serial.printf("Backlight timeout: %d\r\n", backlightTimeout);
    Serial.printf("Backlight while charging: %d\r\n", bBacklightCharging);
    Serial.printf("SealevelPressure (hPa): %f\r\n", sealevelhPa);
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
    Core2.SetBacklightSettings( backlightTimeout, bBacklightCharging );
    installButtonHandlers();
    Screen.Init(currentScreen, DispData);
    SysStatus.ResetBleStatus();
    Screen.ShowSysStatus(SysStatus);
    // reconnect BLE
    if( bBtEnabled )
        LevoBle.Reconnect();
}

void updateTripButtonStatus()
{
    VirtualSensors::enTripStatus tripStatus = VirtSensors.GetTripStatus();
    if (tripStatus == VirtualSensors::STARTED )
    {
        SysStatus.tripStatus = SystemStatus::STARTED;
        Screen.SetButtonBarButtonState(M5Screen::BTSTART, true);
        Screen.SetButtonBarButtonState(M5Screen::BTSTOP, false);
    }
    else if (tripStatus == VirtualSensors::STOPPED )
    {
        SysStatus.tripStatus = SystemStatus::STOPPED;
        Screen.SetButtonBarButtonState(M5Screen::BTSTART, false);
        Screen.SetButtonBarButtonState(M5Screen::BTSTOP, true);
    }
    else if (tripStatus == VirtualSensors::RESET )
    {
        SysStatus.tripStatus = SystemStatus::NONE;
        Screen.SetButtonBarButtonState(M5Screen::BTSTART, false);
        Screen.SetButtonBarButtonState(M5Screen::BTSTOP, false);
    }
}

void onBtTripOrTune(Event& e)
{
    // start: 0, stop: 1, reset: 2, tune: 3
    if( e.button->userData == M5Screen::BTTUNE )
    {
        uninstallButtonHandlers();
        M5ConfigFormTune form(LevoBle);
        Core2.ResetDisplayTimer();
        Screen.Init(currentScreen, DispData);
        installButtonHandlers();
    }
    else if (e.button->userData == M5Screen::BTRESET)
    {
        // LevoBle.Disconnect();
        uninstallButtonHandlers();
        if (M5ConfigForms::MsgBox("Finish tour & reset trip data?", M5ConfigForms::YESNO) == M5ConfigForms::RET_YES)
        {
            VirtSensors.WriteStatisticsSD(DispData);
            VirtSensors.ResetTrip();
        }
        Core2.ResetDisplayTimer();
        Screen.Init(currentScreen, DispData);
        installButtonHandlers();
        // LevoBle.Reconnect(); // todo generates a small fragment of a log file at the end of a trip
    }
    else if (e.button->userData == M5Screen::BTSTART)
        VirtSensors.StartTrip();
    else if (e.button->userData == M5Screen::BTSTOP)
        VirtSensors.StopTrip();
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
    Logger.Writeln(DisplayData::BLE_MOT_PEAKASSIST1, peakAssistVal, DispData, logFormat, timestamp );
    peakAssistVal.fVal = (float)bleVal.raw.data[1];
    Logger.Writeln(DisplayData::BLE_MOT_PEAKASSIST2, peakAssistVal, DispData, logFormat, timestamp );
    peakAssistVal.fVal = (float)bleVal.raw.data[2];
    Logger.Writeln(DisplayData::BLE_MOT_PEAKASSIST3, peakAssistVal, DispData, logFormat, timestamp );
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
        Logger.Writeln(id, bleVal, DispData, logFormat, timestamp );
    }
}

// print and log float values from non BLE sensors
void ShowFloatData(DisplayData::enIds id, float fVal, uint32_t timestamp )
{
    Screen.ShowValue(id, fVal, DispData); // ouput to screen
    // log data and dump to serial
    LevoEsp32Ble::stBleVal bleVal;
    bleVal.unionType = LevoEsp32Ble::FLOAT; bleVal.fVal = fVal;
    Logger.Writeln(id, bleVal, DispData, logFormat, timestamp );
}

void setup ()
{
    Prefs.begin("LevoEsp32", false);

    // M5Core2 system init
    Core2.Init();
    Core2.CheckSDCard(SysStatus);
    delay(500);

    // get settings
    readPreferences();
    Core2.SetBacklightSettings(backlightTimeout, bBacklightCharging);

    // altimeter
    bHasAltimeter = Altimeter.Init();
    if( !bHasAltimeter )
    {
        DispData.Hide(DisplayData::BARO_ALTIMETER );
        DispData.Hide(DisplayData::VIRT_INCLINATION);
        DispData.Hide(DisplayData::TRIP_ELEVATIONGAIN);
    }

    // all screen output
    Screen.SetButtonBarHandler( onBtTripOrTune );
    Screen.Init(M5Screen::SCREEN_A, DispData);

    // bluetooth communication
    LevoBle.Init( ReadBluetoothPin(), bBtEnabled );

    // buttons
    installButtonHandlers();
}

// running on core 1: xPortGetCoreID()
//////////////////////////////////////
void loop ()
{
    static uint32_t ti100  = millis() + 100L;
    static uint32_t ti1000 = millis() + 1000L;

    // call timer once
    uint32_t ti = millis();

    // bluetooth handling
    LevoEsp32Ble::stBleVal bleVal;
    if (LevoBle.Update(bleVal))
        ShowBleData( bleVal, ti );

    // do all 100ms
    if (ti >= ti100)
    {
        // print system status to screen
        ti100 = ti + 100L;
        SysStatus.UpdateBleStatus( LevoBle.GetBleStatus() );
        Core2.CheckPower(SysStatus);
        if (Screen.ShowSysStatus(SysStatus))
            BleStatusChanged();

        // dim display after some time
        Core2.DoDisplayTimer();

        // show state of start/stop/reset button
        updateTripButtonStatus();

        // get virtual sensor values
        DisplayData::enIds id; float fVal;
        if (VirtSensors.Update(id, fVal, ti))
            ShowFloatData(id, fVal, ti);
    }

    // do all 1000ms
    if (ti >= ti1000)
    {
        ti1000 = ti + 1000L;

        // Altimeter
        if (bHasAltimeter)
        {
            float altitude = Altimeter.GetAltitude(sealevelhPa);
            ShowFloatData( DisplayData::BARO_ALTIMETER, altitude, ti );
            VirtSensors.FeedValue(DisplayData::BARO_ALTIMETER, altitude, ti);  // notify virtual sensors
        }
    }

    // touch update
    M5.update();
}
