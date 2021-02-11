/** Levo BLE ESP32 Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 */

#ifndef DISPLAYDATA_H
#define DISPLAYDATA_H

#include "Arduino.h"

class DisplayData
{
public:
    typedef enum
    {
        UNKNOWN         = -1,
        // data delivered by BLE
        BLE_BATT_ENERGY = 0,
        BLE_BATT_HEALTH,
        BLE_BATT_TEMP,
        BLE_BATT_CHARGECYCLES,
        BLE_BATT_VOLTAGE,
        BLE_BATT_CURRENT,
        BLE_BATT_CHARGEPERCENT,
        BLE_RIDER_POWER,
        BLE_MOT_CADENCE,
        BLE_MOT_SPEED,
        BLE_MOT_ODOMETER,
        BLE_MOT_ASSISTLEVEL,
        BLE_MOT_TEMP,
        BLE_MOT_POWER,
        BLE_BIKE_WHEELCIRC,
        // ...in the future: ANT+ and CAN values
        NUM_ELEMENTS // must be the last value
    } enIds;

    typedef struct
    {
        int dataId;
        const char* strLabel;
        const char* strUnit;
        int nWidth;       // including decimal point
        int nPrecision;
    } stDisplayData;

    const stDisplayData* GetDescription(enIds id);
    static const int numElements = NUM_ELEMENTS;
    enIds GetIdFromIdx(int i);

protected:
    const stDisplayData displayData[numElements] =
    {
        // id                     label             unit,   width, precision
        { BLE_BATT_ENERGY,        "Batt energy",    "Wh",   4, 0 },
        { BLE_BATT_HEALTH,        "Batt health",    "%",    4, 0 },
        { BLE_BATT_TEMP,          "Batt temp",      "&o",   4, 0 },// &o = °
        { BLE_BATT_CHARGECYCLES,  "Charge cycle",   "",     4, 0 },
        { BLE_BATT_VOLTAGE,       "Batt voltage",   "V",    4, 1 },
        { BLE_BATT_CURRENT,       "Batt current",   "A",    4, 1 },
        { BLE_BATT_CHARGEPERCENT, "Charge state",   "%",    4, 0 },
        { BLE_RIDER_POWER,        "Rider power",    "W",    4, 0 },
        { BLE_MOT_CADENCE,        "Cadence",        "rpm",  5, 1 },
        { BLE_MOT_SPEED,          "Speed ",         "kmh",  6, 1 },
        { BLE_MOT_ODOMETER,       "Odometer",       "km",   7, 2 },
        { BLE_MOT_ASSISTLEVEL,    "Assist level",   "",     4, 0 },
        { BLE_MOT_TEMP,           "Mot temp",       "&o",   4, 0 }, // &o = °
        { BLE_MOT_POWER,          "Mot power",      "W",    4, 0 },
        { BLE_BIKE_WHEELCIRC,     "Wheel circ.",    "mm",   4, 0 }
    };

    // field order on screen
    const enIds order[numElements] = { BLE_MOT_SPEED,
                                       BLE_RIDER_POWER,
                                       BLE_BATT_CURRENT,
                                       BLE_BATT_VOLTAGE,
                                       BLE_BATT_ENERGY,
                                       BLE_BATT_CHARGEPERCENT,
                                       BLE_MOT_POWER,
                                       BLE_MOT_CADENCE,
                                       BLE_MOT_ASSISTLEVEL,
                                       BLE_MOT_ODOMETER,
                                       BLE_BATT_TEMP,
                                       BLE_MOT_TEMP,
                                       BLE_BIKE_WHEELCIRC,
                                       BLE_BATT_CHARGECYCLES,
                                       BLE_BATT_HEALTH
                                     };
};

#endif // DISPLAYDATA_H