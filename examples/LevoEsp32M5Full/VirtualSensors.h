/*
 *
 *  Created: 24/02/2021
 *      Author: Bernd Wokoeck
 *
 * Virtual sensors:calculate values from other values (i.e. averages, integrals)
 *
 */

#ifndef VIRTUAL_SENSORS_H
#define VIRTUAL_SENSORS_H

#undef min
#include <deque>
#include "DisplayData.h"

class VirtualSensors
{
public:
    VirtualSensors();

    void FeedValue( DisplayData::enIds id, float fVal, uint32_t timestamp ); // value from any other sensor
    bool Update( DisplayData::enIds & id, float& fVal, uint32_t timestamp);  // poll virtual sensor values

    void StartTrip(DisplayData& DispData);
    void StopTrip();
    void ResetTrip();

    bool WriteStatisticsSD(DisplayData& DispData);

    typedef enum {
        STOPPED = 0,
        STARTED = 1,
        RESET   = 3,
    } enTripStatus;
    enTripStatus m_tripStatus = RESET;

protected:
    // inclination calculation
    class InclinationQueue
    {
    public:
        InclinationQueue() : m_cnt(0), m_head(0), m_tail(0) {}
        bool PopTail(float& odo, float& alti);
        bool PushHead(float odo, float alti);
        bool IsFull() { return (m_cnt == SIZE); }
        bool IsEmpty() { return (m_cnt == 0); }
    protected:
        struct stInclination
        {
            float odo;
            float alti;
        };
        enum{ SIZE = 10 };
        stInclination m_values[SIZE];
        int  m_cnt;
        size_t m_head;
        size_t m_tail;
    };
    InclinationQueue m_queue;

    // value history 
    float m_lastOdoValue = 0.0;
    float m_lastAltitude = 0.0;
    float m_lastMotPower = 0.0;
    float m_lastBattTemp = 0.0;
    float m_lastSpeed    = 0.0;

    // start stop time values
    uint32_t m_startTime = 0L;
    uint32_t m_stopTime  = 0L;

    // trip values - abstract base class
    struct stVirtSensorValue
    {
        stVirtSensorValue() : bChanged(false), lastTime(0) {}
        bool bChanged; // indicate that value can be processed by Update()
        uint32_t lastTime;
        virtual void stop() = 0;
        virtual void reset() = 0;
        virtual void setValue(float fVal, uint32_t timestamp) = 0;
        virtual bool deliverValue( float & fVal, bool bForce = false ) = 0;
    };
    struct stAbsDifferenceValue : stVirtSensorValue
    {
        stAbsDifferenceValue() : pastTripValueSum(0.0), currentTripValue(0.0), startValue(0.0) {}
        float    pastTripValueSum;  // sum of values for last start/stop period
        float    currentTripValue;  // current value
        float    startValue;        // startValue since last start()
        virtual void stop();
        virtual void reset() { currentTripValue = pastTripValueSum =  startValue = 0.0; }
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue( float & fVal, bool bForce = false );
    };
    struct stPeakValue : stVirtSensorValue
    {
        stPeakValue() : pastTripValueSum(0.0), currentTripValue(0.0), startValue(0.0) {}
        float    pastTripValueSum;  // sum of values for last start/stop period
        float    currentTripValue;  // current value
        float    startValue;        // startValue since last start()
        virtual void stop();
        virtual void reset() { currentTripValue = pastTripValueSum = startValue = 0.0; }
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue(float& fVal, bool bForce = false);
    };
    struct stMinValue : stVirtSensorValue
    {
        stMinValue() : pastTripValueSum(0.0), currentTripValue(0.0), startValue(0.0) {}
        float    pastTripValueSum;  // sum of values for last start/stop period
        float    currentTripValue;  // current value
        float    startValue;        // startValue since last start()
        virtual void stop();
        virtual void reset() { currentTripValue = pastTripValueSum = startValue = 0.0; }
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue(float& fVal, bool bForce = false);
    };
    struct stSumupPositiveValue : stVirtSensorValue
    {
        stSumupPositiveValue() : pastTripValueSum(0.0), currentTripValue(0.0), startValue(0.0) {}
        float    pastTripValueSum;  // sum of values for last start/stop period
        float    currentTripValue;  // current value
        float    startValue;        // startValue since last start()
        virtual void stop();
        virtual void reset() { currentTripValue = pastTripValueSum = startValue = 0.0; }
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue(float& fVal, bool bForce = false);
    };
    struct stIntegrationValue : stVirtSensorValue
    {
        stIntegrationValue() : pastTripValueSum(0.0), currentTripValue(0.0) { lastTime = 0; }
        float    pastTripValueSum;  // sum of values for last start/stop period
        float    currentTripValue;  // current value
        virtual void stop();
        virtual void reset() { currentTripValue = pastTripValueSum = 0.0; lastTime = 0; }
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue(float& fVal, bool bForce = false);
    };
    struct stAverageValue : stVirtSensorValue
    {
        stAverageValue() : currentTripValue(0.0), sumTime(0.0) { lastTime = 0; }
        float    currentTripValue;  // current value
        float    sumTime;
        virtual void stop();
        virtual void reset() { currentTripValue = sumTime = 0.0; lastTime = 0; }
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue(float& fVal, bool bForce = false);
    };
    struct stSimpleValue : stVirtSensorValue
    {
        stSimpleValue() : value(0.0) {}
        float value;
        virtual void stop() {}
        virtual void reset() {}
        virtual void setValue(float fVal, uint32_t timestamp);
        virtual bool deliverValue(float& fVal, bool bForce = false);
    };
    stVirtSensorValue* m_sensorValues[DisplayData::numElements] = {0};

    void updateTripTime(uint32_t timestamp);
    void setValue( DisplayData::enIds id, float fVal, uint32_t timestamp);
    int  m_currentId  = DisplayData::BLE_BATT_SIZEWH;

    void calcInclination(float fOdo, float fAlti, uint32_t timestamp );
    void calcConsumption(float fSpeed, uint32_t timestamp);
    void formatAsTime(float val, size_t nLen, char* strVal);
    void refreshTripDisplay(DisplayData& DispData);
};

#endif // VirtualSensors