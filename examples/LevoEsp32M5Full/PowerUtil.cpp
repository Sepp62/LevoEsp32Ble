/*
 *
 *  Created: 04/04/2021
 *      Author: Bernd Wokoeck
 *
 * Power Calc: All calculations for power and calibration etc
 *
 */

#include <M5Core2.h>
#include <Preferences.h>
#include "DisplayData.h"
#include "PowerUtil.h"

void PowerUtil::DumpEta(float eta)
{
    // open file in append mode
    File dataFile = SD.open("/EtaLog.txt", FILE_APPEND);
    if (dataFile)
    {
        dataFile.printf("%f\r\n", eta );
        dataFile.close();
    }
    Serial.printf("calculated eta: %f\r\n", eta );
}

void PowerUtil::DumpPoints(const char* title, BikeResistance::measurePoints& p, double cR, double cwA)
{
    int i;

    if (p.size() == 0)
        return;

    double speedMultiplier = (p[0].y > 10.0) ? 1.0 : 3.6; // dirty hack: automatic km/h or m/s conversion

    // file logging - filename is <date>.cal
    char filename[20];
    RTC_TimeTypeDef RTC_Time;
    RTC_DateTypeDef RTC_Date;
    M5.Rtc.GetDate(&RTC_Date);
    M5.Rtc.GetTime(&RTC_Time);
    snprintf(filename, sizeof(filename), "/%02d%02d%02d.cal", RTC_Date.Date, RTC_Date.Month, (uint8_t)(RTC_Date.Year - 2000));

    // open file in append mode
    File dataFile = SD.open(filename, FILE_APPEND);
    if (dataFile)
    {
        dataFile.printf("%s - %02d:%02d:%02d\r\n", title, RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
        for (i = 0; i < p.size(); ++i)
            dataFile.printf("%f\t%f\r\n", p[i].x, p[i].y * speedMultiplier);
        if (cR >= 0.0 && cwA >= 0.0)
            dataFile.printf("cR: %f\tcwA: %f\r\n", cR, cwA);
        dataFile.close();
    }

    // serial logging
    Serial.printf("%s\r\n", title);
    for (i = 0; i < p.size(); ++i)
        Serial.printf("%f\t%f\r\n", p[i].x, p[i].y * speedMultiplier);
    if (cR >= 0.0 && cwA >= 0.0)
        Serial.printf("cR: %f\tcwA: %f\r\n", cR, cwA);
}

void PowerUtil::SysParamsInit(Preferences& prefs)
{
    m_sysParams.mass             = prefs.getFloat("Sp_mass", 110.0 );
    m_sysParams.cR               = prefs.getFloat("Sp_cR", 0.009725 );
    m_sysParams.cwA              = prefs.getFloat("Sp_cwA", 0.437392);
    m_sysParams.avgRiderPower    = prefs.getFloat("Sp_avgRdPower", 100.0 );
    m_sysParams.defaultAirTemp   = prefs.getFloat("Sp_defAirTemp", 18.0 );
    m_sysParams.defaultAltitude  = prefs.getFloat("Sp_defAlt", 520.0);
    m_sysParams.nCalibrationRuns = prefs.getUChar("Sp_nRuns", 4 );
    m_sysParams.eta              = prefs.getFloat("Sp_eta", 0.6 );
    m_bikePower.SetSystemParams( m_sysParams.mass, m_sysParams.cR, m_sysParams.cwA );

    Serial.printf( "Sp_mass: %f\r\n",       m_sysParams.mass );
    Serial.printf( "Sp_cR: %f\r\n",         m_sysParams.cR );
    Serial.printf( "Sp_cwA: %f\r\n",        m_sysParams.cwA );
    Serial.printf( "Sp_avgRdPower: %f\r\n", m_sysParams.avgRiderPower );
    Serial.printf( "Sp_defAirTemp: %f\r\n", m_sysParams.defaultAirTemp );
    Serial.printf( "Sp_defAlt: %f\r\n",     m_sysParams.defaultAltitude );
    Serial.printf( "Sp_nRuns: %d\r\n",      m_sysParams.nCalibrationRuns );
    Serial.printf( "Sp_eta: %f\r\n",        m_sysParams.eta );

    // write timestamp to etalog.txt
    File dataFile = SD.open("/EtaLog.txt", FILE_APPEND);
    if (dataFile)
    {
        RTC_TimeTypeDef RTC_Time;
        RTC_DateTypeDef RTC_Date;
        M5.Rtc.GetDate(&RTC_Date);
        M5.Rtc.GetTime(&RTC_Time);
        dataFile.printf("--- %02d.%02d.%04d %02d:%02d:%02d --- \r\n", RTC_Date.Date, RTC_Date.Month, RTC_Date.Year, RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
        dataFile.close();
    }
}

void PowerUtil::CalibrationSave( Preferences& prefs, float cR, float cwA )
{
    m_sysParams.cR  = cR;
    m_sysParams.cwA = cwA;
    prefs.putFloat("Sp_cR",  m_sysParams.cR);
    prefs.putFloat("Sp_cwA", m_sysParams.cwA);
}

void PowerUtil::EtaSave(Preferences& prefs, float eta)
{
    m_sysParams.eta = eta;
    prefs.putFloat("Sp_eta", m_sysParams.eta);
    // Serial.printf("eta saved: %f\r\n", eta );
}

void PowerUtil::EnableCalibrationMode(bool bEnable )
{
    int i;
    m_calibrationState = bEnable ? WAITING : INACTIVE;
    if (bEnable)
        clearAll();
}

void PowerUtil::clearAll()
{
    int i;

    // delete all runs
    for( i = 0; i < m_runs.size(); ++i )
        m_runs[i].clear();
    m_runs.clear();
    m_exactPoints.clear();
}

bool PowerUtil::GetCalibrationResult(float& cR, float& cwA)
{
    if( m_calibrationState == READY && m_cRResult > 0.0 )
    {
        m_calibrationState = INACTIVE;
        cR  = m_cRResult;
        cwA = m_cwAResult;
        clearAll();
        return true;
    }
    return false;
}

// values from other sensors
void PowerUtil::FeedValue(DisplayData::enIds id, float fVal, uint32_t timestamp)
{
    if( id == DisplayData::BLE_MOT_SPEED )
    {
        if ( m_calibrationState ==  WAITING )
        {
            // speed must be falling from > 25 km/h and inclination ~ 0
            // cadence and motor power cannot be used to detect roll phase since both are delayed
            if( (m_lastSpeed >= 25.0 &&  fVal < 25.0) && abs( m_lastInclinationPercent ) < 1.0 )
            {
                m_calibrationState = RUNNING;
                m_lastDistance = 0.0;
                m_nIncline = 0;
                m_runs.push_back( BikeResistance::measurePoints() );
                m_exactPoints.clear();
            }
        }

        if( m_calibrationState == RUNNING )
        {
            bool bPedalling = (m_lastCadence > 10.0) && (m_lastDistance > 30.0); // cadence value is delayed

            // speed dejittering
            if( fVal > m_lastSpeed )
                m_nIncline++;
            else
                m_nIncline = 0;

            // invalid roll conditions
            if( bPedalling || m_nIncline > 1 || m_lastDistance > 300.0 )
            {
                m_calibrationState = ABORTED;
                m_runs.back().clear();
                return;
            }

            // minimum speed reached --> ready
            if (fVal < 3.5)
            {
                // calibration run finished --> success
                Serial.printf("PowerUtil::RUN READY\r\n");
                DumpPoints("Exact", m_exactPoints);
                if (m_runs.size() > 0)
                    DumpPoints( "Interpolated", m_runs.back() );

                // all runs passed?
                if( m_runs.size() >= m_sysParams.nCalibrationRuns )
                {
                    m_calibrationState = READY;
                    calibrationCalc(m_cRResult, m_cwAResult);
                }
                else
                    m_calibrationState = WAITING; // next run
            }
            else
            {
                // valid measurement
                uint32_t ti = timestamp - m_lastSpeedTime;
                float s = m_lastSpeed/3.6 * (float)ti/1000.0; // distance since last speed point
                float newDistance = m_lastDistance + s;
                m_exactPoints.push_back( LevMarq::point_t(newDistance, fVal ) ); // just for debugging

                // interpolate to integer speed values for easy average calculation and data reduction
                int nSpan = floor(m_lastSpeed) - floor(fVal);
                if( nSpan > 0 ) // if speed crosses integer boundary, also makes sure that fval < m_lastspeed
                {
                    for( int i = 0; i < nSpan; i++ )
                    {
                        // x = (x2-x1) * (y-y1) / (y2-y1) + x1
                        float speed = floor( m_lastSpeed - i * 1.0 );
                        float dist  = ((( newDistance - m_lastDistance ) * ( speed - m_lastSpeed )) / ( fVal - m_lastSpeed ) ) + m_lastDistance;

                        // let first speed begin with zero distance
                        if( m_runs.back().size() == 0 )
                            m_zeroDistance = dist;

                        // add tuple distance/speed to measurement table
                        m_runs.back().push_back(LevMarq::point_t( dist - m_zeroDistance, speed ));
                    }
                }
                m_lastDistance = newDistance;
            }
        }

        // auto recover from abort state
        if (m_calibrationState == ABORTED )
        {
            if( fVal >= 25.0 )
                m_calibrationState = WAITING;
        }    

        m_lastSpeed = fVal;
        m_lastSpeedTime = timestamp;
    }
    else
    {
        // store "last" values
        switch (id)
        {
        case DisplayData::BLE_MOT_CADENCE:   m_lastCadence = fVal;            break;
        case DisplayData::BLE_MOT_ODOMETER:  m_lastOdoValue = fVal;           break;
        case DisplayData::BARO_TEMP:         m_lastAirTemp = fVal;            break;
        case DisplayData::BARO_ALTIMETER:    m_lastAltitude = fVal;           break;
        case DisplayData::BLE_MOT_POWER:     m_lastMotPower = fVal;           break;
        case DisplayData::BLE_RIDER_POWER:   m_lastRiderPower = fVal;         break;
        case DisplayData::VIRT_INCLINATION:  m_lastInclinationPercent = fVal; break;
        case DisplayData::BLE_BATT_REMAINWH: m_lastRemainWh = fVal;           break;
        }
    }
}

// calc correction factor between measured electric energy and rider energy and calculated energy
// in theory this is "eta" aka efficiency, in practice it is a try to correct measurement errors
void PowerUtil::calcEfficiency(float calcPower, uint32_t timestamp)
{
    // start value for battery level - begin of ride
    if( m_startRemainWh == 0 && m_lastRemainWh != 0 )
        m_startRemainWh = m_lastRemainWh;

    // energy calc for rider and battery
    if( m_lastUpdateTime != 0 )
    {
        float timeHours = (float)(timestamp - m_lastUpdateTime) / (1000.0 * 3600.0);
        m_riderEnergy += m_lastRiderPower * timeHours;
        if (calcPower >= 0.0 ) // sum up positive energy values W = P * t (Wh)
            m_calcEnergy  += calcPower * timeHours;
    }

    if (m_startRemainWh != 0 )
    {
        uint32_t battEnergy = m_startRemainWh - m_lastRemainWh;
        if( battEnergy >= 100 ) // after some time calculate eta as correction factor
        {
            m_sysParams.eta = (m_calcEnergy - m_riderEnergy) / (float)battEnergy;
            // Serial.printf("usedBattery: %ld, calcEnergy: %f, riderEnergy: %f, eta: %f\r\n", battEnergy, m_calcEnergy, m_riderEnergy, m_eta);

            if( (battEnergy % 50) == 0 )
            {
                if( !m_bEtaWritten )
                {
                    DumpEta(m_sysParams.eta);
                    m_bEtaWritten = m_bEtaValid = true;
                }
            }
            else
                m_bEtaWritten = false;
        }
    }
}


// poll PowerUtil sensor values
bool PowerUtil::Update(DisplayData::enIds& id, float& fVal, uint32_t timestamp)
{
    // curent power
    fVal = m_bikePower.CurrentPower( m_lastSpeed/3.6, m_lastInclinationPercent, m_lastAltitude, m_lastAirTemp, timestamp );
    id   = DisplayData::PWR_POWER;

    // efficiency
    calcEfficiency( fVal, timestamp);

    m_lastUpdateTime = timestamp;
    return true;
}

PowerUtil::enCalibrationState PowerUtil::CheckCalibration()
{
  // future options:
  // if ready: return nRun, maxDist, maxSpeed, minSpeed
  // if running: currentSpeed or percent ready
  return m_calibrationState;
}

bool PowerUtil::calibrationCalc(float& cR, float& cwA)
{
    bool ret = false;
    cR = 0.0; cwA = 0.0;

    if( m_runs.size() == 0 )
        return false;

    // find max speed over all runs
    int i, j, nEle = 0;
    for( i = 0; i < m_runs.size(); i++ )
    {
        int y = m_runs[i][0].y + .5;
        nEle = (nEle > y) ? nEle : y;
    }

    // distance values to be summed up for averaging, index is speed
    std::vector<double> x( nEle + 1, 0.0 );
    std::vector<int> nx( nEle + 1, 0 );

    // output vector
    BikeResistance::measurePoints avg;

    // sum up over all runs
    for( i = 0; i < m_runs.size(); ++i )
    {
        for( j = 0; j < m_runs[i].size(); ++j )
        {
            int idx = m_runs[i][j].y + .5; // integer value of speed is used as index to distance table
            x[idx] += m_runs[i][j].x;      // sum
            nx[idx]++;                     // number of values
        }
    }
    // calc average and store to output vector
    for( i = x.size() - 1; i >= 0; --i )
    {
        if( nx[i] == m_runs.size() )
        {
            // Serial.printf("nx: %d, x: %f, i: %d\r\n", nx[i], x[i], i );
            avg.push_back( LevMarq::point_t( x[i] / nx[i], ((double)i)/3.6 ) ); // speed now in m/s!
        }
    }
    // remove unused elements
    while( avg.back().x == 0.0 )
      avg.pop_back();

    // calc resistance values
    double dcR, dcwA;
    BikeResistance br;
    br.SetSystemParams( m_sysParams.mass, m_lastAirTemp, m_lastAltitude );
    if( br.CalcResistance( dcR, dcwA, avg ) )
    {
        DumpPoints("average", avg, dcR, dcwA);
        cR = dcR;
        cwA = dcwA;
        ret = true;
    }
    else
    {
        DumpPoints("calc error - average", avg );
    }

    return ret;
}
