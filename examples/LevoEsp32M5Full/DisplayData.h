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
        VIRT_INCLINATION,
        TRIP_DISTANCE,
        TRIP_TIME,
        TRIP_AVGSPEED,
        TRIP_RIDERENERGY,
        TRIP_MOTORENERGY,
        TRIP_BATTENERGY,
        TRIP_ELEVATIONGAIN,
        TRIP_PEAKMOTTEMP,
        TRIP_PEAKBATTTEMP,
        TRIP_PEAKBATTCURRENT,
        TRIP_PEAKRIDERPOWER,
        TRIP_PEAKMOTORPOWER,
        TRIP_MINBATTVOLTAGE,
        TRIP_MAXSPEED,
        TRIP_CONSUMPTION,
        VIRT_CONSUMPTION,

        NUM_ELEMENTS // must be the last value
    } enIds;

    typedef enum
    {   // used for logging
        DYNAMIC = 1, // dynamic value value, sporadically changes
        STATIC = 2,  // values, which normally do not change 
        TRIP = 4,    // trip and average values
        TIME = 8,    // format as time
    } enFlags;

    typedef struct
    {
        int dataId;
        const char* strLabel;
        const char* strUnit;
        int nWidth;       // including decimal point
        int nPrecision;
        int flags;
    } stDisplayData;

    const stDisplayData* GetDescription(enIds id);
    static const int numElements = NUM_ELEMENTS;

    void Hide(enIds id) { hiddenMask.set(id); } // hide a value forever

protected:
    const stDisplayData displayData[numElements] =
    {
        // id                     label             unit,   width, precision,   flags
        { BLE_BATT_SIZEWH,        "Batt size",      "Wh",   4, 0,               STATIC  },
        { BLE_BATT_REMAINWH,      "Batt energy",    "Wh",   4, 0,               DYNAMIC },
        { BLE_BATT_HEALTH,        "Batt health",    "%",    4, 0,               STATIC  },
        { BLE_BATT_TEMP,          "Batt temp",      "&o",   4, 0,               DYNAMIC },// &o = °
        { BLE_BATT_CHARGECYCLES,  "Charge cycle",   "",     4, 0,               STATIC  },
        { BLE_BATT_VOLTAGE,       "Batt voltage",   "V",    4, 1,               DYNAMIC },
        { BLE_BATT_CURRENT,       "Batt current",   "A",    4, 1,               DYNAMIC },
        { BLE_BATT_CHARGEPERCENT, "Charge state",   "%",    4, 0,               DYNAMIC },

        { BLE_RIDER_POWER,        "Rider power",    "W",    4, 0,               DYNAMIC },
        { BLE_MOT_CADENCE,        "Cadence",        "rpm",  5, 1,               DYNAMIC },
        { BLE_MOT_SPEED,          "Speed ",         "kmh",  5, 1,               DYNAMIC },
        { BLE_MOT_ODOMETER,       "Odometer",       "km",   7, 2,               DYNAMIC },
        { BLE_MOT_ASSISTLEVEL,    "Assist level",   "",     4, 0,               DYNAMIC },
        { BLE_MOT_TEMP,           "Mot temp",       "&o",   4, 0,               DYNAMIC }, // &o = °
        { BLE_MOT_POWER,          "Mot power in",   "W",    4, 0,               DYNAMIC },
        { BLE_MOT_PEAKASSIST1,    "Peak Eco",       "%",    4, 0,               STATIC  },
        { BLE_MOT_PEAKASSIST2,    "Peak Trail",     "%",    4, 0,               STATIC  },
        { BLE_MOT_PEAKASSIST3,    "Peak Turbo",     "%",    4, 0,               STATIC  },
        { BLE_MOT_SHUTTLE,        "Shuttle",        "%",    4, 0,               STATIC  },

        { BLE_BIKE_WHEELCIRC,     "Wheel circ.",    "mm",   4, 0,               STATIC  },
        { BLE_BIKE_ASSISTLEV1,    "Assist Eco",     "%",    4, 0,               STATIC  },
        { BLE_BIKE_ASSISTLEV2,    "Assist Trail",   "%",    4, 0,               STATIC  },
        { BLE_BIKE_ASSISTLEV3,    "Assist Turbo",   "%",    4, 0,               STATIC  },
        { BLE_BIKE_FAKECHANNEL,   "Fake chan",      "",     4, 0,               STATIC  },
        { BLE_BIKE_ACCEL,         "Accel.",         "%",    4, 0,               STATIC  },

        { BARO_ALTIMETER,         "Elevation",      "m",    4, 0,               DYNAMIC },
        { VIRT_INCLINATION,       "Inclination",    "%",    3, 0,               DYNAMIC },
        { TRIP_DISTANCE,          "Trip dist.",     "km",   7, 2,               TRIP    },
        { TRIP_TIME,              "Trip time",      "",     5, 0,               TRIP|TIME },
        { TRIP_AVGSPEED,          "Avg speed",      "kmh",  5, 1,               TRIP    },
        { TRIP_RIDERENERGY,       "Rider energy",   "Wh",   4, 0,               TRIP    },
        { TRIP_MOTORENERGY,       "Motor energy",   "Wh",   4, 0,               TRIP    },
        { TRIP_BATTENERGY,        "Batt energy",    "Wh",   4, 0,               TRIP    },
        { TRIP_ELEVATIONGAIN,     "Elev. gain",     "m",    4, 0,               TRIP    },
        { TRIP_PEAKMOTTEMP,       "Peak Tmot",      "&o",   4, 0,               TRIP    },
        { TRIP_PEAKBATTTEMP,      "Peak Tbatt",     "&o",   4, 0,               TRIP    },
        { TRIP_PEAKBATTCURRENT,   "Peak Ibatt",     "A",    4, 1,               TRIP    },
        { TRIP_PEAKRIDERPOWER,    "Peak Prider",    "W",    4, 0,               TRIP    },
        { TRIP_PEAKMOTORPOWER,    "Peak Pmot",      "W",    4, 0,               TRIP    },
        { TRIP_MINBATTVOLTAGE,    "Min bat volt",   "V",    4, 1,               TRIP    },
        { TRIP_MAXSPEED,          "Max speed",      "kmh",  5, 1,               TRIP    },
        { TRIP_CONSUMPTION,       "Trip Wh/km",     "",     4, 1,               TRIP    },
        { VIRT_CONSUMPTION,       "Motor Wh/km",    "",     4, 1,               DYNAMIC },
    };

    std::bitset<numElements> hiddenMask;
};

#endif // DISPLAYDATA_H