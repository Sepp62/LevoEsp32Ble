/** Levo BLE ESP32 Simple serial output sample
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 */

 // secret bluetooth pin
#define PASS_KEY 000000 // set your LEVO bluetooth pin here (6 digits)

#include <LevoEsp32Ble.h>

LevoEsp32Ble LevoBle;

void ShowBleData( LevoEsp32Ble::stBleVal & bleVal )
{
    std::string strLog = "UNKNOWN: ";

    // Ble data type to internal display id
    switch (bleVal.dataType)
    {
    case LevoEsp32Ble::BATT_ENERGY:        strLog = "Batt energy (Wh): "; break;
    case LevoEsp32Ble::BATT_HEALTH:        strLog = "Batt health (%): "; break;
    case LevoEsp32Ble::BATT_TEMP:          strLog = "Batt temp (Deg.): "; break;
    case LevoEsp32Ble::BATT_CHARGECYCLES:  strLog = "Charge cycles: "; break;
    case LevoEsp32Ble::BATT_VOLTAGE:       strLog = "Batt voltage ? (V): "; break;
    case LevoEsp32Ble::BATT_CURRENT:       strLog = "Batt current ? (A): "; break;
    case LevoEsp32Ble::BATT_CHARGEPERCENT: strLog = "Charge state (%): "; break;
    case LevoEsp32Ble::RIDER_POWER:        strLog = "Rider power (W): "; break;
    case LevoEsp32Ble::MOT_CADENCE:        strLog = "Cadence (rpm)): "; break;
    case LevoEsp32Ble::MOT_SPEED:          strLog = "Speed (km/h): "; break;
    case LevoEsp32Ble::MOT_ODOMETER:       strLog = "Odometer (km): "; break;
    case LevoEsp32Ble::MOT_ASSISTLEVEL:    strLog = "Assist level: "; break;
    case LevoEsp32Ble::MOT_TEMP:           strLog = "Motor temp (Deg.): "; break;
    case LevoEsp32Ble::MOT_POWER:          strLog = "Motor power (W): "; break;
    case LevoEsp32Ble::BIKE_WHEELCIRC:     strLog = "Whell circ. (mm): "; break;
    }

    if (bleVal.dataType != LevoEsp32Ble::UNKNOWN)
    {
        // decoded value
        if( bleVal.unionType == LevoEsp32Ble::FLOAT )
        {
            Serial.print(strLog.c_str());
            Serial.println(bleVal.fVal);
        }
    }
    else
    {
        // undecoded/raw output
        if (bleVal.unionType == LevoEsp32Ble::BINARY)
        {
            char s[] = "00 ";
            for (int i = 0; i < bleVal.raw.len; i++)
            {
                snprintf(s, sizeof(s), "%02x ", bleVal.raw.data[i]);
                strLog += s;
            }
            Serial.println(strLog.c_str());
        }
    }
}

void setup ()
{
    Serial.begin(115200);
    LevoBle.Init( PASS_KEY );
}

// running on core 1: xPortGetCoreID()
//////////////////////////////////////
void loop ()
{
    // bluetooth handling
    LevoEsp32Ble::stBleVal bleVal;
    if (LevoBle.Update(bleVal))
        ShowBleData(bleVal);

    delay(1);
}
