/** Levo BLE ESP32 Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 */

#ifndef DISPLAYDATA_H
#define DISPLAYDATA_H

#undef min // min macro conflicts with bitset
#include <bitset>
#include "Arduino.h"

class DisplayData
{
public:
    typedef enum
    {
        UNKNOWN         = -1,
        // data delivered by BLE
        BLE_BATT_SIZEWH = 0,
        BLE_BATT_REMAINWH,
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
        BLE_MOT_PEAKASSIST1,
        BLE_MOT_PEAKASSIST2,
        BLE_MOT_PEAKASSIST3,
        BLE_MOT_SHUTTLE,

        BLE_BIKE_WHEELCIRC,
        BLE_BIKE_ASSISTLEV1,
        BLE_BIKE_ASSISTLEV2,
        BLE_BIKE_ASSISTLEV3,
        BLE_BIKE_FAKECHANNEL,
        BLE_BIKE_ACCEL,
        // ...in the future: Elevation, GPS, ANT+, CAN and other values
        BARO_ALTIMETER,

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

    void Hide(enIds id) { hiddenMask.set(id); } // hide a value forever

protected:
    const stDisplayData displayData[numElements] =
    {
        // id                     label             unit,   width, precision
        { BLE_BATT_SIZEWH,        "Batt size",      "Wh",   4, 0 },
        { BLE_BATT_REMAINWH,      "Batt energy",    "Wh",   4, 0 },
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
        { BLE_MOT_PEAKASSIST1,    "Peak Eco",       "%",    4, 0 },
        { BLE_MOT_PEAKASSIST2,    "Peak Trail",     "%",    4, 0 },
        { BLE_MOT_PEAKASSIST3,    "Peak Turbo",     "%",    4, 0 },
        { BLE_MOT_SHUTTLE,        "Shuttle",        "%",    4, 0 },

        { BLE_BIKE_WHEELCIRC,     "Wheel circ.",    "mm",   4, 0 },
        { BLE_BIKE_ASSISTLEV1,    "Assist Eco",     "%",    4, 0 },
        { BLE_BIKE_ASSISTLEV2,    "Assist Trail",   "%",    4, 0 },
        { BLE_BIKE_ASSISTLEV3,    "Assist Turbo",   "%",    4, 0 },
        { BLE_BIKE_FAKECHANNEL,   "Fake chan",      "",     4, 0 },
        { BLE_BIKE_ACCEL,         "Accel.",         "%",    4, 0 },

        { BARO_ALTIMETER,         "Elevation",      "m",    4, 0 },
    };

    std::bitset<numElements> hiddenMask;
};

#endif // DISPLAYDATA_H