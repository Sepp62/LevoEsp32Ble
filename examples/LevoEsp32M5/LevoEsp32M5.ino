/** Levo BLE ESP32 Data Logger on M5Core2
 
 *      Author: Bernd Wokoeck
 *
 *  Displays Specialized Levo 2019+ telemetry values to M5Core2 display
 *  and logs data to SD card (levolog.txt)
 *  Enter Bluetooth pin to code below or generate pin.txt on SD card with pin inside
 * 
 *  Images converted with: https://lvgl.io/tools/imageconverter (True color, C-Array)
 *
 *  Currently not used, but probably helpful: https://github.com/M5ez/Core2ez
 *
 *      History:
 *      -------------------------------------------------------------
 *      0.90   01/01/2021  created, example for M5Core2 hardware 
 *
 */

// secret bluetooth pin
#define PASS_KEY 000000 // set your LEVO bluetooth pin here or write it to pin.txt on SD card

#include <LevoEsp32Ble.h>
#include "DisplayData.h"
#include "SystemStatus.h"
#include "M5System.h"
#include "M5Screen.h"
#include "FileLogger.h"

// tick count for uncritical loop funtions
static const unsigned long TICK_MS = 100L;

LevoEsp32Ble LevoBle;
DisplayData  DispData;
M5System     Core2;
M5Screen     Screen;
SystemStatus SysStatus;
FileLogger   Logger;

// Read bluetooth pin from SD card file "pin.txt"
uint32_t ReadBluetoothPin()
{
    // if programmatically set, use it
    if(PASS_KEY != 0 )
        return PASS_KEY;

    File     dataFile;
    uint32_t pin = 0L;

    sdcard_type_t Type = SD.cardType();
    if (Type == CARD_UNKNOWN || Type == CARD_NONE)
    {
        Serial.println("No SD card.");
        return 0L;
    }

    dataFile = SD.open("/pin.txt", FILE_READ);
    if (!dataFile)
    {
        Serial.println("Error opening log file.");
        return 0L;
    }

    while (dataFile.available())
    {
        pin = dataFile.parseInt();
    }
    Serial.print("BT Pin read from SD: ");
    Serial.println(pin);
    dataFile.close();
    return pin;
}


void BleStatusChanged()
{
    if (LevoBle.GetBleStatus() == LevoEsp32Ble::CONNECTED)
    {
        // open log file
        Logger.Open();
    }
    else
    {
        // flush log file
        Logger.Close();
    }
}


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

    std::string strLog;
    if (id != DisplayData::UNKNOWN)
    {
        // decoded value
        if( bleVal.unionType == LevoEsp32Ble::FLOAT )
        {
            Screen.ShowValue( id, bleVal.fVal, DispData); // ouput to screen
            DispData.LogDump(id, bleVal.fVal, strLog );   // format for log output
        }
    }
    else
    {
        // undecoded/raw for log output
        if (bleVal.unionType == LevoEsp32Ble::BINARY)
        {
            strLog = "UNKNOWN\t";
            DispData.HexDump( (char*)bleVal.raw.data, bleVal.raw.len, strLog );
        }
    }

    // log data here
    Serial.println(strLog.c_str());
    Logger.Writeln(strLog.c_str());
}

void setup ()
{
    // Serial.begin(115200); dont do, when M5 is used.

    // M5Core2 system init
    Core2.Init();
    delay(500);

    // all screen output
    Screen.Init(DispData);
    
    // bluetooth communication
    LevoBle.Init( ReadBluetoothPin() );
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

    // print system status from time to time
    if (millis() > ti)
    {
        ti = millis() + TICK_MS;
        SysStatus.UpdateBleStatus( LevoBle.GetBleStatus() );
        Core2.CheckPower(SysStatus);
        if (Screen.ShowSysStatus(SysStatus))
            BleStatusChanged();
    }

    delay(1);
}
