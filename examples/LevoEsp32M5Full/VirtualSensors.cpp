/*
 *
 *  Created: 24/02/2021
 *      Author: Bernd Wokoeck
 *
 * Virtual sensors:calculate values from other values (i.e. averages, integrals)
 *
 */

#include <M5Core2.h>
#include "VirtualSensors.h"

// todo TRIP_MOTORENERGY and TRIP_BATTENERGY should be the same, invent TRIP_CONSUMPTION (Wh/km) instead

VirtualSensors::VirtualSensors()
{
    m_sensorValues[DisplayData::VIRT_INCLINATION]     = new stSimpleValue;
    m_sensorValues[DisplayData::VIRT_CONSUMPTION]     = new stSimpleValue;
    m_sensorValues[DisplayData::TRIP_CONSUMPTION]     = new stSimpleValue;
    m_sensorValues[DisplayData::TRIP_TIME]            = new stAbsDifferenceValue;
    m_sensorValues[DisplayData::TRIP_DISTANCE]        = new stAbsDifferenceValue;
    m_sensorValues[DisplayData::TRIP_AVGSPEED]        = new stAverageValue;
    m_sensorValues[DisplayData::TRIP_RIDERENERGY]     = new stIntegrationValue;
    m_sensorValues[DisplayData::TRIP_MOTORENERGY]     = new stIntegrationValue;
    m_sensorValues[DisplayData::TRIP_BATTENERGY]      = new stAbsDifferenceValue;
    m_sensorValues[DisplayData::TRIP_ELEVATIONGAIN]   = new stSumupPositiveValue;
    m_sensorValues[DisplayData::TRIP_PEAKMOTTEMP]     = new stPeakValue;
    m_sensorValues[DisplayData::TRIP_PEAKBATTTEMP]    = new stPeakValue;
    m_sensorValues[DisplayData::TRIP_PEAKBATTCURRENT] = new stPeakValue;
    m_sensorValues[DisplayData::TRIP_PEAKRIDERPOWER]  = new stPeakValue;
    m_sensorValues[DisplayData::TRIP_PEAKMOTORPOWER]  = new stPeakValue;
    m_sensorValues[DisplayData::TRIP_MINBATTVOLTAGE]  = new stMinValue;
    m_sensorValues[DisplayData::TRIP_MAXSPEED]        = new stPeakValue;
}

void VirtualSensors::StartTrip()
{
    m_tripStatus = STARTED;
    m_startTime = millis();

    // needs special treatment since this value changes only from time to time and trip will not be started yet at power on 
    setValue( DisplayData::TRIP_PEAKBATTTEMP, m_lastBattTemp, m_startTime );
}

void VirtualSensors::StopTrip()
{
    int i;
    m_tripStatus = STOPPED;
    m_stopTime = millis();

    for (i = 0; i < DisplayData::numElements; i++)
    {
        DisplayData::enIds id = (DisplayData::enIds)i;
        stVirtSensorValue* pValue = m_sensorValues[id];
        if (pValue)
            pValue->stop();
    }
}

void VirtualSensors::ResetTrip()
{
    int i;
    m_tripStatus = RESET;
    m_startTime = m_stopTime = 0;
    for (i = 0; i < DisplayData::numElements; i++)
    {
        DisplayData::enIds id = (DisplayData::enIds)i;
        stVirtSensorValue* pValue = m_sensorValues[id];
        if (pValue)
            pValue->reset();
    }
}

bool VirtualSensors::WriteStatisticsSD(DisplayData& DispData)
{
    // filename
    char filename[20];
    RTC_DateTypeDef RTC_Date;
    RTC_TimeTypeDef RTC_Time;
    M5.Rtc.GetDate(&RTC_Date);
    M5.Rtc.GetTime(&RTC_Time);
    snprintf(filename, sizeof( filename), "/%02.2d%02.2d%02.2d.txt", RTC_Date.Date, RTC_Date.Month, RTC_Date.Year);

    // check card
    sdcard_type_t Type = SD.cardType();
    if (Type == CARD_UNKNOWN || Type == CARD_NONE)
    {
        Serial.println("No SD card!");
        return false;
    }

    // open file in append mode
    File dataFile = SD.open(filename, FILE_APPEND);
    if (!dataFile)
    {
        Serial.printf("SD: can't open: %s\r\n", filename );
        return false;
    }

    // headline
    char headline[80];
    snprintf(headline, sizeof(headline), "----- Trip stats - %02d.%02d.%04d, %02d:%02d:%02d -----", RTC_Date.Date, RTC_Date.Month, RTC_Date.Year, RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
    Serial.println(headline);
    dataFile.println(headline);

    // write all trip values to SD
    char strFile[80] = "";
    for (int i = 0; i < DisplayData::numElements; i++)
    {
        float fVal;
        DisplayData::enIds id = (DisplayData::enIds)i;
        stVirtSensorValue* pValue = m_sensorValues[id];
        if (pValue && pValue->deliverValue( fVal, true ) )
        {
            const DisplayData::stDisplayData* pDesc = DispData.GetDescription(id);
            if (pDesc == 0)
                continue;

            // write only trip values
            if ( (pDesc->flags & DisplayData::TRIP) == 0 )
                continue;

            // format time or float value with correct precision
            char strVal[20];
            if( pDesc->flags & DisplayData::TIME )
                formatAsTime( fVal, sizeof(strVal), strVal );
            else
                dtostrf( fVal, 7, pDesc->nPrecision, strVal );

            // replace degree symbol
            char strUnit[10];
            strncpy(strUnit, pDesc->strUnit, sizeof(strUnit));
            if (strUnit[0] == '&' && strUnit[1] == 'o')
            {
                strUnit[0] = '\xb0';
                strUnit[1] = '\0';
            }

            // log to file
            snprintf(strFile, sizeof(strFile), "%s:\t%s %s", pDesc->strLabel, strVal, strUnit);
            Serial.println(strFile);
            dataFile.println(strFile);
        }
    }

    dataFile.close();
    return true;
}

void VirtualSensors::stAbsDifferenceValue::stop()
{
    pastTripValueSum += currentTripValue;
    currentTripValue = 0.0;
    startValue = 0.0;
}

void VirtualSensors::stSumupPositiveValue::stop()
{
    pastTripValueSum += currentTripValue;
    currentTripValue = 0.0;
    startValue = 0.0;
}

void VirtualSensors::stPeakValue::stop()
{
    pastTripValueSum = max( currentTripValue, pastTripValueSum );
    currentTripValue = 0.0;
    startValue = 0.0;
}

void VirtualSensors::stMinValue::stop()
{
    if(pastTripValueSum == 0.0 )
        pastTripValueSum = currentTripValue;
    else
        pastTripValueSum = min( currentTripValue, pastTripValueSum );
    currentTripValue = startValue;
    startValue = 0.0;
}

void VirtualSensors::stIntegrationValue::stop()
{
    pastTripValueSum += currentTripValue;
    currentTripValue = 0.0;
}

void VirtualSensors::stAverageValue::stop()
{
    lastTime = 0;
}

bool VirtualSensors::stAbsDifferenceValue::deliverValue( float& fVal, bool bForce )
{
    if ( bChanged || bForce )
    {
        fVal = currentTripValue + pastTripValueSum;
        bChanged = false;
        return true;
    }
    return false;
}

bool VirtualSensors::stSumupPositiveValue::deliverValue(float& fVal, bool bForce )
{
    if (bChanged || bForce)
    {
        fVal = currentTripValue + pastTripValueSum;
        bChanged = false;
        return true;
    }
    return false;
}

bool VirtualSensors::stPeakValue::deliverValue(float& fVal, bool bForce )
{
    if (bChanged || bForce)
    {
        fVal = max( currentTripValue, pastTripValueSum );
        bChanged = false;
        return true;
    }
    return false;
}

bool VirtualSensors::stMinValue::deliverValue(float& fVal, bool bForce)
{
    if (bChanged || bForce)
    {
        if(pastTripValueSum == 0.0 )
            fVal = currentTripValue;
        else
            fVal = min(currentTripValue, pastTripValueSum);
        bChanged = false;
        return true;
    }
    return false;
}

// timebase "hours" (i.e. Wh)
bool VirtualSensors::stIntegrationValue::deliverValue(float& fVal, bool bForce)
{
    if (bChanged || bForce)
    {
        fVal = (currentTripValue + pastTripValueSum)/3600.0; // from seconds to hours
        bChanged = false;
        return true;
    }
    return false;
}

bool VirtualSensors::stAverageValue::deliverValue(float& fVal, bool bForce)
{
    if (bChanged || bForce)
    {
        if( sumTime != 0.0 )
            fVal = currentTripValue/sumTime;
        else
            fVal = 0.0;
        bChanged = false;
        return true;
    }
    return false;
}

bool VirtualSensors::stSimpleValue::deliverValue(float& fVal, bool bForce)
{
    if (bChanged || bForce)
    {
        fVal = value;
        bChanged = false;
        return true;
    }
    return false;
}

// for all values
void VirtualSensors::setValue(DisplayData::enIds id, float fVal, uint32_t timestamp)
{
    if (m_tripStatus == STARTED)
    {
        stVirtSensorValue* pValue = m_sensorValues[id];
        if (pValue)
            pValue->setValue(fVal, timestamp);
    }
}

// speed, batt temp, mot temp, mot current 
void VirtualSensors::stPeakValue::setValue(float fVal, uint32_t timestamp)
{
    if (startValue == 0.0)
        startValue = fVal;
    currentTripValue = max( fVal, currentTripValue );
    lastTime = timestamp;
    bChanged = true;
}

// batt voltage
void VirtualSensors::stMinValue::setValue(float fVal, uint32_t timestamp)
{
    if (startValue == 0.0)
        startValue = currentTripValue = fVal;
    currentTripValue = min(fVal, currentTripValue);
    lastTime = timestamp;
    bChanged = true;
}

// elevation gain
void VirtualSensors::stSumupPositiveValue::setValue( float fVal, uint32_t timestamp )
{
    if (startValue == 0.0)
        startValue = fVal;
    float newVal = fVal - startValue;
    if( newVal > 0.0 )
        currentTripValue += newVal;
    startValue = fVal;
    lastTime = timestamp;
    bChanged = true;
}

// trip distance and consumed battery energy
void VirtualSensors::stAbsDifferenceValue::setValue( float fVal, uint32_t timestamp )
{
    if( startValue == 0.0 )
        startValue = fVal;
    currentTripValue = abs( fVal - startValue );
    lastTime = timestamp;
    bChanged = true;
}

// rider and motor energy
void VirtualSensors::stIntegrationValue::setValue(float fVal, uint32_t timestamp)
{
    if(currentTripValue == 0.0 )
        lastTime = timestamp; // we will loose the first value after start
    float integrationTime = (timestamp - lastTime)/1000.0; // sec
    currentTripValue += fVal * integrationTime;
    lastTime = timestamp;
    bChanged = true;
}

// speed
void VirtualSensors::stAverageValue::setValue(float fVal, uint32_t timestamp)
{
    if( lastTime = 0 )
        lastTime = timestamp; // we will loose the first value after start
    float integrationTime = (timestamp - lastTime)/1000.0; // sec
    sumTime += integrationTime;
    currentTripValue += fVal * integrationTime;
    lastTime = timestamp;
    bChanged = true;
}

// power consumption, altitude
void VirtualSensors::stSimpleValue::setValue(float fVal, uint32_t timestamp)
{
    value = fVal;
    lastTime = timestamp;
    bChanged = true;
}

// trip time
void VirtualSensors::updateTripTime(uint32_t timestamp)
{
    stAbsDifferenceValue* pValue = static_cast<stAbsDifferenceValue*>(m_sensorValues[DisplayData::TRIP_TIME]);
    if (pValue)
    {
        if (m_tripStatus == STARTED && timestamp > (pValue->lastTime + 1000))
        {
            if (pValue->startValue == 0.0)
                pValue->startValue = (float)m_startTime / 1000.0;
            pValue->currentTripValue = (float)timestamp / 1000.0 - pValue->startValue;
            pValue->lastTime = timestamp;
            pValue->bChanged = true;
        }
    }
}

void VirtualSensors::calcConsumption(float fSpeed, uint32_t timestamp)
{
    // VIRT_CONSUMPTION
    float consumption = 0.0;

    if( fSpeed > 3.0 )
        consumption = m_lastMotPower / fSpeed;

    stVirtSensorValue* pValue = m_sensorValues[DisplayData::VIRT_CONSUMPTION];
    if (pValue)
        pValue->setValue(consumption, timestamp);

    // TRIP_CONSUMPTION
    stVirtSensorValue* pValueMotEnergy    = m_sensorValues[DisplayData::TRIP_MOTORENERGY];
    stVirtSensorValue* pValueTripDistance = m_sensorValues[DisplayData::TRIP_DISTANCE];
    if (pValueMotEnergy && pValueTripDistance)
    {  
        float motEnergy, tripDistance;
        if (pValueMotEnergy->deliverValue(motEnergy, true) && pValueTripDistance->deliverValue(tripDistance, true))
        {
            stVirtSensorValue* pValueTripConsumption = m_sensorValues[DisplayData::TRIP_CONSUMPTION];
            if( pValueTripConsumption && tripDistance != 0.0 )
                pValueTripConsumption->setValue(motEnergy/tripDistance, timestamp );
        }
    }
}

void VirtualSensors::calcInclination(float fOdoVal, uint32_t timestamp)
{
    m_lastOdoValue = fOdoVal;
    stInclination newInc(fOdoVal, m_lastAltitude);
    m_queue.emplace_front(newInc); // add new
    if (m_queue.size() >= 10) // at least 100.0m
    {
        stInclination first(m_queue.front());
        stInclination last(m_queue.back());
        while (m_queue.size() >= 10)
            m_queue.pop_back();  // remove last entries until 9 values remain

        // exact calculation (prone to exceptions)
        // float roadDistance = (last.odoValue - first.odoValue) * 1000;        // distance converted to meter
        // float deltaY = last.altitude - first.altitude;                       // elevation difference
        // float deltaX = sqrt(roadDistance * roadDistance - deltaY * deltaY ); // horizontal projection of roadDistance (pythagoras)
        // approx calculation (error ~1.3% at 30%)
        float deltaY = last.altitude - first.altitude;                 // elevation difference
        float deltaX = (last.odoValue - first.odoValue) * 1000;        // distance converted to meter
        if (deltaX > 0.0)
        {
            float inclination = round( deltaY / deltaX * 100 );
            setValue(DisplayData::VIRT_INCLINATION, inclination, timestamp );
            // Serial.printf("calc inclination: %f %%\r\n", inclination);
        }
    }
}

// val is in seconds
void VirtualSensors::formatAsTime(float val, size_t nLen, char* strVal)
{
    int h, m, s;
    h = val / 3600.0;
    m = (val - h * 3600.0) / 60.0;
    s = val - h * 3600.0 - m * 60.0;
    snprintf(strVal, nLen, "%02.2d:%02.2d:%02.2d", h, m, s);
}

// feed values from a "physical" sensor and calc dependant virtual values 
void VirtualSensors::FeedValue(DisplayData::enIds id, float fVal, uint32_t timestamp )
{
    if (id == DisplayData::BARO_ALTIMETER)
    {
        setValue(DisplayData::TRIP_ELEVATIONGAIN, fVal, timestamp); // BMP280 has noise which leads to a drift of ~20hm/h 
        m_lastAltitude = fVal;
    }
    else if (id == DisplayData::BLE_MOT_ODOMETER)
    {
        setValue(DisplayData::TRIP_DISTANCE, fVal, timestamp);
        if (fVal != m_lastOdoValue) // has changed for at least 10 m
            calcInclination( fVal, timestamp );
    }
    else if (id == DisplayData::BLE_BATT_REMAINWH)
    {
        setValue(DisplayData::TRIP_BATTENERGY, fVal, timestamp);
    }
    else if (id == DisplayData::BLE_BATT_TEMP)
    {
        setValue(DisplayData::TRIP_PEAKBATTTEMP, fVal, timestamp);
        m_lastBattTemp = fVal;
    }
    else if (id == DisplayData::BLE_BATT_VOLTAGE)
    {
        setValue(DisplayData::TRIP_MINBATTVOLTAGE, fVal, timestamp);
    }
    else if (id == DisplayData::BLE_BATT_CURRENT)
    {
        setValue(DisplayData::TRIP_PEAKBATTCURRENT, fVal, timestamp);
    }
    else if (id == DisplayData::BLE_RIDER_POWER)
    {
        setValue(DisplayData::TRIP_PEAKRIDERPOWER, fVal, timestamp);
        setValue(DisplayData::TRIP_RIDERENERGY, fVal, timestamp);
    }
    else if (id == DisplayData::BLE_MOT_SPEED)
    {
        setValue(DisplayData::TRIP_MAXSPEED, fVal, timestamp);
        setValue(DisplayData::TRIP_AVGSPEED, fVal, timestamp);
        calcConsumption(fVal, timestamp);
    }
    else if (id == DisplayData::BLE_MOT_TEMP)
    {
        setValue(DisplayData::TRIP_PEAKMOTTEMP, fVal, timestamp);
    }
    else if (id == DisplayData::BLE_MOT_POWER)
    {
        setValue(DisplayData::TRIP_PEAKMOTORPOWER, fVal, timestamp);
        setValue(DisplayData::TRIP_MOTORENERGY, fVal, timestamp);
        m_lastMotPower = fVal;
    }
}

// called approx. all 100ms from main loop to poll new trip values
bool VirtualSensors::Update(DisplayData::enIds& id, float& fVal, uint32_t timestamp)
{
    // special handling for trip time
    updateTripTime(timestamp);

    int cnt = 0;
    // round robin for all values that have changed
    while( cnt++ < DisplayData::numElements)
    {
        stVirtSensorValue* pValue = m_sensorValues[(DisplayData::enIds)m_currentId];
        if (pValue && pValue->bChanged && pValue->deliverValue( fVal ) )
        {
            id = (DisplayData::enIds)m_currentId;
            return true;
        }
        if( ++m_currentId >= DisplayData::numElements )
            m_currentId = DisplayData::BLE_BATT_SIZEWH;
    }

    return false;
}
