/*
 *
 *  Created: 04/04/2021
 *      Author: Bernd Wokoeck
 *
 * Power Calc: All calculations for power and calibration etc
 *
 */

#ifndef POWERUTIL_H
#define POWERUTIL_H

#include <BikePowerCalc.h>
#include <Preferences.h>
#include "DisplayData.h"

class PowerUtil
{
public:
    typedef enum
    {
        INACTIVE = 0,
        WAITING,
        RUNNING,
        READY,
        ABORTED,
    } enCalibrationState;

    struct stSystemParams
    {
        float   mass;             // system mass: m_bike + m_rider + m_wheel (approximation for inertial momentum) 
        float   cR;               // coeff for roll resistance
        float   cwA;              // coeff for air resistance * surface
        float   avgRiderPower;    // rider power
        float   defaultAirTemp;   // air temperature to calculate rho when no sensor data available
        float   defaultAltitude;  // altitude to calculate rho when no sensor data available
        uint8_t nCalibrationRuns; // number of calibration runs
        float   eta;              // compensation factor for electric effiency and calibration parameter error 
    };

    // system parameters
    void SysParamsInit( Preferences & prefs );
    void CalibrationSave(Preferences& prefs, float cR, float cwA );
    void EtaSave(Preferences& prefs, float eta);

    // calibration 
    void EnableCalibrationMode( bool bEnable ); // start/stop calibration mode
    enCalibrationState CheckCalibration();
    bool GetCalibrationResult(float& cR, float& cwA);

    // power correction factor (compensates eletric efficiency and calibration parameter error: calcPower = (motorPower * eta) + riderPower) 
    bool GetEta(float& eta) { if( m_bEtaValid ) { eta = m_sysParams.eta; m_bEtaValid = false;  return true; } return false; }

    // sensor value receipt and delivery 
    void FeedValue(DisplayData::enIds id, float fVal, uint32_t timestamp); // value from any other sensor
    bool Update(DisplayData::enIds& id, float& fVal, uint32_t timestamp);  // poll PowerUtil sensor values

    // todo
    // sensor values: current calculated power for a point via feed/update
    // calculations: energy for a gpx route / plot position -> altitude/energy for a route

protected:

    enCalibrationState m_calibrationState = INACTIVE;

    stSystemParams m_sysParams = 
    {
        110.0,
        0.009725,
        0.437392,
        100.0,
        18.0,
        520.0,
        4,
        0.6,
    };

    // for calibration
    float    m_lastCadence = 0.0;
    float    m_lastSpeed = 0.0;
    uint32_t m_lastSpeedTime = 0;
    float    m_lastDistance = 0.0;
    float    m_lastOdoValue = 0.0; // for plausi check only, distance is determined by speed and time
    float    m_lastMotPower = 0.0;
    float    m_lastRiderPower = 0.0;

    // measurement tables
    typedef std::vector<BikeResistance::measurePoints> run_t; // measurePoints for a couple of runs
    run_t m_runs;
    float m_zeroDistance = 0.0; // averaged dist/speed table gets offset to be zero for first entry
    void clearAll();

    // speed dejittering
    int m_nIncline = 0;

    // resistance calculation and result
    float m_cRResult;
    float m_cwAResult;
    bool calibrationCalc(float& cR, float& cwA);

    // debugging and logging
    BikeResistance::measurePoints m_exactPoints;
    void DumpPoints( const char * title, BikeResistance::measurePoints & p, double cR = -1.0, double cwA = -1.0 );
    void DumpEta( float eta );

    // for power calculation
    BikePower m_bikePower;
    float     m_lastInclinationPercent = 0.0;
    float     m_lastAirTemp = m_sysParams.defaultAirTemp;
    float     m_lastAltitude = m_sysParams.defaultAltitude;

    // energy and efficiency calculation
    float    m_calcEnergy     = 0.0;
    float    m_riderEnergy    = 0.0;
    uint32_t m_lastRemainWh   = 0;
    uint32_t m_startRemainWh  = 0;
    uint32_t m_lastUpdateTime = 0;
    bool     m_bEtaWritten = false;
    bool     m_bEtaValid   = false;
    void calcEfficiency( float calcPower, uint32_t timestamp );
};

#endif // POWERUTIL_H