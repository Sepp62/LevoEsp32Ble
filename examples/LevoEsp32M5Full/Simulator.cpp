/*
 *
 *  Created: 13/03/2021
 *      Author: Bernd Wokoeck
 *
 * Replay a log file to simulate a trip
 *
 */

#include <M5Core2.h>
#include "Simulator.h"

// Time;   Dist; Id; Batt energy; Batt temp; Batt voltage; Batt current; Charge state; Rider power; Cadence; Speed; Odometer; Assist level; Mot temp; Mot power in; Elevation; Inclination; Motor Wh / km; Temp
// 1564.8; 8.22; 29; 370;         21;        38.2;         6.8;          74;           115;         84.7;    25;    3837.92;  3;            32;       266;          613;       0.0;         12.22;          16.9

// indicates # of column for a specific id, 0 = invalid
const int Simulator::s_valColArray[] =
{
   0,   // BLE_BATT_SIZEWH,
   3,   // BLE_BATT_REMAINWH,
   0,   // BLE_BATT_HEALTH,
   4,   // BLE_BATT_TEMP,
   0,   // BLE_BATT_CHARGECYCLES,
   5,   // BLE_BATT_VOLTAGE,
   6,   // BLE_BATT_CURRENT,
   7,   // BLE_BATT_CHARGEPERCENT,
   8,   // BLE_RIDER_POWER,
   9,   // BLE_MOT_CADENCE,
  10,   // BLE_MOT_SPEED,
  11,   // BLE_MOT_ODOMETER,
  12,   // BLE_MOT_ASSISTLEVEL,
  13,   // BLE_MOT_TEMP,
  14,   // BLE_MOT_POWER,
   0,   // BLE_MOT_PEAKASSIST1,
   0,   // BLE_MOT_PEAKASSIST2,
   0,   // BLE_MOT_PEAKASSIST3,
   0,   // BLE_MOT_SHUTTLE,
   0,   // BLE_BIKE_WHEELCIRC,
   0,   // BLE_BIKE_ASSISTLEV1,
   0,   // BLE_BIKE_ASSISTLEV2,
   0,   // BLE_BIKE_ASSISTLEV3,
   0,   // BLE_BIKE_FAKECHANNEL,
   0,   // BLE_BIKE_ACCEL,
  15,   // BARO_ALTIMETER,
  16,   // VIRT_INCLINATION,
   0,   // TRIP_DISTANCE,
   0,   // TRIP_TIME,
   0,   // TRIP_AVGSPEED,
   0,   // TRIP_RIDERENERGY,
   0,   // TRIP_MOTORENERGY,
   0,   // TRIP_BATTENERGY,
   0,   // TRIP_ELEVATIONGAIN,
   0,   // TRIP_PEAKMOTTEMP,
   0,   // TRIP_PEAKBATTTEMP,
   0,   // TRIP_PEAKBATTCURRENT,
   0,   // TRIP_PEAKRIDERPOWER,
   0,   // TRIP_PEAKMOTORPOWER,
   0,   // TRIP_MINBATTVOLTAGE,
   0,   // TRIP_MAXSPEED,
   0,   // TRIP_CONSUMPTION,
  17,   // VIRT_CONSUMPTION,
   0,   // TRIP_RANGE,
   0,   // GYRO_PITCH,
  18,   // BARO_TEMP,
};

bool Simulator::readLine(uint32_t& time, int& dispId, float& fVal)
{
    int colCnt, valCol;
    char buffer[250];
    uint32_t ti;
    int id;

    while (m_simFile.available())
    {
        size_t nRead = m_simFile.readBytesUntil('\n', buffer, sizeof( buffer) );
        if ( nRead >= 0 )
        {
            colCnt = 0;
            valCol = 0;

            buffer[nRead] = '\0';
            char* token = strtok(buffer, "\t;");
            while (token != NULL)
            {
                // Serial.println( token );
                // time in ms
                if( colCnt == 0 )
                {
                    ti = (uint32_t)(atof( token ) * 1000.0);
                }
                // value id
                else if( colCnt == 2 )
                {
                    id = atoi( token );
                    if( id >= sizeof( s_valColArray ) )
                        return false;
                    if( ( valCol = s_valColArray[id] ) == 0 )
                        return false;
                }
                // get value from column
                else if (valCol == colCnt)
                {
                    time = ti;
                    dispId = id;
                    fVal = atof( token );
                    return true;
                }
                token = strtok(NULL, "\t;");
                colCnt++;
            }
        }
    }
    return false;
}

bool Simulator::Update( DisplayData::enIds& id, float& fVal, uint32_t timestamp)
{
    sdcard_type_t Type = SD.cardType();
    if (Type == CARD_UNKNOWN || Type == CARD_NONE)
        return false;

    if (!m_simFile)
    {
        m_simFile = SD.open("/simulator.txt", FILE_READ );
        m_startTime = timestamp;
        uint32_t ti; int id; float val;
        if (readLine(ti, id, val))
            m_startTimeOffset = ti; // eliminate time offset in file
    }

    if (m_simFile)
    {
        if( m_nextValueTime == 0 )
        {
            if( readLine(m_nextValueTime, m_nextValueId, m_nextValue) )
            {
                // Serial.printf("time: %ld, id: %d, val: %f\r\n", m_nextValueTime, m_nextValueId, m_nextValue );
                m_nextValueTime += m_startTime - m_startTimeOffset;
            }
        }
        else if( timestamp > m_nextValueTime )
        {
            m_nextValueTime = 0;
            id   = (DisplayData::enIds)m_nextValueId;
            fVal = m_nextValue;
            return true;
        }
    }

    return false;
}

