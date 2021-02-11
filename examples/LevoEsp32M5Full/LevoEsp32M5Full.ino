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
 *  https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences
 *  https://github.com/tuupola/bm8563
 * 
 *  Images converted with: https://lvgl.io/tools/imageconverter (True color, C-Array)
 *
 *      History:
 *      -------------------------------------------------------------
 *      0.90   01/01/2021  created, example for M5Core2 hardware 
 *      0.91   04/02/2021  added: config forms, backlight dim
 *      0.92   07/02/2021  csv log formats added
 *
 */

#include <Preferences.h>
#include <LevoEsp32Ble.h>
#include "DisplayData.h"
#include "SystemStatus.h"
#include "M5System.h"
#include "M5Screen.h"
#include "FileLogger.h"

// tick count for uncritical loop funtions
static const unsigned long TICK_MS = 100L;

Preferences  Prefs;    
LevoEsp32Ble LevoBle;
DisplayData  DispData;
M5System     Core2;
M5Screen     Screen;
SystemStatus SysStatus;
FileLogger   Logger;

// local settings
FileLogger::enLogFormat logFormat = FileLogger::SIMPLE; // CSV_TABLE; // CSV_KNOWN; // CSV_SIMPLE; // SIMPLE;
bool     bBtEnabled = true;
uint16_t backlightTimeout = 60;
bool     bBacklightCharging = true;

// read all settings to local settings
void readPreferences()
{
    logFormat          = (FileLogger::enLogFormat)Prefs.getUChar("LogFormat");
    bBtEnabled         = Prefs.getUChar("BtEnabled", 1) ? true : false;
    backlightTimeout   = Prefs.getUShort("BacklightTo", 60);
    bBacklightCharging = Prefs.getUChar("BacklightChg", 1) ? true : false;

    Serial.printf("Log format: %d\r\n", logFormat);
    Serial.printf("Bluetooth enabled: %d\r\n", bBtEnabled);
    Serial.printf("Backlight timeout: %d\r\n", backlightTimeout);
    Serial.printf("Backlight while charging: %d\r\n", bBacklightCharging);
}

// Read bluetooth pin from preferences
uint32_t ReadBluetoothPin()
{
    uint32_t pin = Prefs.getULong("BtPin", 0);
    SysStatus.bHasBtPin = (pin != 0) ? true : false;
    return pin;
}

// settings button
Button btSettings(220, 0, 100, 60); // top right corner
void onBtSettings(Event& e)
{
    Serial.println("Show settings");
    LevoBle.Disconnect();
    btSettings.hide();
    // Show menu
    Screen.ShowConfig(Prefs);
    // refresh settings
    readPreferences();
    // reinit display
    Core2.ResetDisplayTimer();
    Core2.SetBacklightSettings( backlightTimeout, bBacklightCharging );
    Screen.Init(DispData);
    btSettings.draw();
    SysStatus.ResetBleStatus();
    Screen.ShowSysStatus(SysStatus);
    // reconnect BLE
    if( bBtEnabled )
        LevoBle.Reconnect();
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
        // flush log file
        Logger.Close();
        SysStatus.bLogging = false;
    }
}

// print and log received bluetooth data
void ShowBleData( LevoEsp32Ble::stBleVal & bleVal )
{
    DisplayData::enIds id = DisplayData::UNKNOWN;

    // Ble data type to internal display id
    switch (bleVal.dataType)
    {
    case LevoEsp32Ble::BATT_ENERGY:        id = DisplayData::BLE_BATT_ENERGY; break;
    case LevoEsp32Ble::BATT_HEALTH:        id = DisplayData::BLE_BATT_HEALTH; break;
    case LevoEsp32Ble::BATT_TEMP:          id = DisplayData::BLE_BATT_TEMP; break;
    case LevoEsp32Ble::BATT_CHARGECYCLES:  id = DisplayData::BLE_BATT_CHARGECYCLES; break;
    case LevoEsp32Ble::BATT_VOLTAGE:       id = DisplayData::BLE_BATT_VOLTAGE; break;
    case LevoEsp32Ble::BATT_CURRENT:       id = DisplayData::BLE_BATT_CURRENT; break;
    case LevoEsp32Ble::BATT_CHARGEPERCENT: id = DisplayData::BLE_BATT_CHARGEPERCENT; break;
    case LevoEsp32Ble::RIDER_POWER:        id = DisplayData::BLE_RIDER_POWER; break;
    case LevoEsp32Ble::MOT_CADENCE:        id = DisplayData::BLE_MOT_CADENCE; break;
    case LevoEsp32Ble::MOT_SPEED:          id = DisplayData::BLE_MOT_SPEED; break;
    case LevoEsp32Ble::MOT_ODOMETER:       id = DisplayData::BLE_MOT_ODOMETER; break;
    case LevoEsp32Ble::MOT_ASSISTLEVEL:    id = DisplayData::BLE_MOT_ASSISTLEVEL; break;
    case LevoEsp32Ble::MOT_TEMP:           id = DisplayData::BLE_MOT_TEMP; break;
    case LevoEsp32Ble::MOT_POWER:          id = DisplayData::BLE_MOT_POWER; break;
    case LevoEsp32Ble::BIKE_WHEELCIRC:     id = DisplayData::BLE_BIKE_WHEELCIRC; break;
    }

    // display decoded value to screen
    if( id != DisplayData::UNKNOWN && bleVal.unionType == LevoEsp32Ble::FLOAT )
        Screen.ShowValue(id, bleVal.fVal, DispData); // ouput to screen

    // log data and dump to serial
    Logger.Writeln(id, bleVal, DispData, logFormat);
}

void setup ()
{
    // Serial.begin(115200); dont do, when M5 is used.

    Prefs.begin("LevoEsp32", false);

    // M5Core2 system init
    Core2.Init();
    Core2.CheckSDCard(SysStatus);
    delay(500);

    // get settings
    readPreferences();
    Core2.SetBacklightSettings(backlightTimeout, bBacklightCharging);

    // all screen output
    Screen.Init(DispData);
    
    // bluetooth communication
    LevoBle.Init( ReadBluetoothPin(), bBtEnabled );

    // settings button
    btSettings.addHandler(onBtSettings, E_TOUCH /*E_TAP*/ );
    M5.Buttons.draw();
}

// running on core 1: xPortGetCoreID()
//////////////////////////////////////
void loop ()
{
    static unsigned long ti = millis() + TICK_MS;

    // bluetooth handling
    LevoEsp32Ble::stBleVal bleVal;
    if (LevoBle.Update(bleVal))
        ShowBleData(bleVal);

    // do some things from time to time
    if (millis() > ti)
    {
        // print system status to screen
        ti = millis() + TICK_MS;
        SysStatus.UpdateBleStatus( LevoBle.GetBleStatus() );
        Core2.CheckPower(SysStatus);
        if (Screen.ShowSysStatus(SysStatus))
            BleStatusChanged();

        // dim display after some time
        Core2.DoDisplayTimer();
    }
    
    // touch update
    M5.update();
}
