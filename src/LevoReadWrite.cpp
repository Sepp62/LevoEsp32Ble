/** Levo ESP32 BLE Data Dumper
 *
 *  Created: 15/02/2021
 *      Author: Bernd Woköck
 *
 *  Enhances LevoEsp32Ble with read/write functionality
 */

#include <M5Core2.h>
#include "LevoReadWrite.h"

// set all elements to -1
void LevoReadWrite::resetAssistData(stLevoAssist& sd )
{
    memset(sd.assist, -1, sizeof(sd.assist ) );
    memset(sd.peakAssist, -1, sizeof(sd.peakAssist));
    sd.shuttle = sd.accelSens = sd.fakeChannel = -1;
}

// check if all values contain valid values
bool LevoReadWrite::isValidAssistData(stLevoAssist& sd)
{
    if( sd.assist[0]     == -1 || sd.assist[1]     == -1 || sd.assist[2]     == -1  || 
        sd.peakAssist[0] == -1 || sd.peakAssist[1] == -1 || sd.peakAssist[2] == -1  ||
        sd.shuttle       == -1 || sd.accelSens     == -1 || sd.fakeChannel   == -1 )
        return false;

    return true;
}

// set current assist level
bool LevoReadWrite::WriteAssistLevel(enAssistLevel enLevel)
{
    if( !IsConnected() )
        return false;

    Serial.println("WriteAssistLevel");

    if (enLevel >= 0 && enLevel <= ASSIST_TURBO)
    {
        uint8_t buf[] = { 0x01, 0x05, 0x00 };
        buf[2] = enLevel;
        writeData(buf, 3);
        return true;
    }
}

// get current assist level
LevoReadWrite::enAssistLevel LevoReadWrite::GetAssistLevel()
{
    enAssistLevel ret = INVALID;
    if (IsConnected())
    {
        stBleVal bleVal;
        if (ReadSync(MOT_ASSISTLEVEL, bleVal, 1000L))
            ret = (enAssistLevel)((int8_t)bleVal.fVal);
    }
    return ret;
}

// read support data synchronuous
bool LevoReadWrite::ReadAssistDataFields(stLevoAssist& sd)
{
    resetAssistData(sd);

    if (!IsConnected())
        return false;

    uint32_t timeout = 0L;

    bool bResubscribe = IsSubscribed();
    Unsubscribe();

    Serial.println("ReadAssistDataFields");

    int i;
    for (i = 0; i < sizeof(m_assistDataFields)/sizeof(enLevoBleDataType); )
    {
        timeout = millis() + 1000L;
        if (ReadAsync(m_assistDataFields[i], true))
        {
            // Serial.printf("Request 0x%04.4x\r\n", assistDataFields[i] );
            stBleVal bleVal;
            while( 1 )
            {
                if (Update(bleVal))
                {
                    if( bleVal.dataType == m_assistDataFields[i])
                    {
                        // Serial.printf("Got 0x%04.4x\r\n", bleVal.dataType);
                        switch (bleVal.dataType)
                        {
                        case MOT_PEAKASSIST:    memcpy(sd.peakAssist, bleVal.raw.data, stLevoAssist::NUM_LEVELS); break;
                        case MOT_SHUTTLE:       sd.shuttle     = (int8_t)bleVal.fVal; break;
                        case BIKE_ASSISTLEV1:   sd.assist[0]   = (int8_t)bleVal.fVal; break;
                        case BIKE_ASSISTLEV2:   sd.assist[1]   = (int8_t)bleVal.fVal; break;
                        case BIKE_ASSISTLEV3:   sd.assist[2]   = (int8_t)bleVal.fVal; break;
                        case BIKE_FAKECHANNEL:  sd.fakeChannel = (int8_t)bleVal.fVal; break;
                        case BIKE_ACCEL:        sd.accelSens   = (int8_t)bleVal.fVal; break;
                        }
                        i++;
                        break;
                    }
                }

                if( millis() > timeout )
                   break;
                delay(1);
            }
        }
    }

    if( bResubscribe )
        Subscribe();

    return isValidAssistData( sd );
}

// write support data synchronuous
bool LevoReadWrite::WriteAssistDataFields(stLevoAssist& sd, uint16_t writeMask )
{
    if (!IsConnected())
        return false;

    int i;
    bool bResubscribe = IsSubscribed();
    Unsubscribe();

    Serial.println("WriteAssistDataFields");

    // peak assistance
    if (  writeMask & BIT_PEAKASSIST &&
          sd.peakAssist[0] >= 0   && sd.peakAssist[1] >= 0   && sd.peakAssist[2] >= 0 &&
          sd.peakAssist[0] <= 100 && sd.peakAssist[1] <= 100 && sd.peakAssist[2] <= 100 )
    {
        uint8_t buf[] = { 0x01, 0x10, 0x00, 0x00, 0x00, 0x32 };
        memcpy( &buf[2], sd.peakAssist, stLevoAssist::NUM_LEVELS );
        writeData( buf, 6);
    }

    // assistance
    if (writeMask & BIT_ASSIST )
    {
        uint8_t buf[] = { 0x02, 0x03, 0x00 };
        for( i = 0; i < stLevoAssist::NUM_LEVELS; i++ )
        {
            if (sd.assist[i] >= 0 && sd.assist[i] <= 100 )
            {
                buf[1] = i + 3;
                buf[2] = sd.assist[i];
                writeData(buf, 3);
            }
        }
    }

    // fake channel
    if ( writeMask & BIT_FAKECHAN && sd.fakeChannel >= 0 && sd.fakeChannel <= 100)
    {
        uint8_t buf[] = { 0x02, 0x06, 0x00 };
        buf[2] = sd.fakeChannel;
        writeData(buf, 3);
    }

    // shuttle
    if ( writeMask & BIT_SHUTTLE && sd.shuttle >= 0 && sd.shuttle <= 100 )
    {
        uint8_t buf[] = { 0x01, 0x15, 0x00 };
        buf[2] = sd.shuttle;
        writeData(buf, 3);
    }

    // accelaration
    if (writeMask & BIT_ACCELSENS && sd.accelSens >= 0 && sd.accelSens <= 100 )
    {
        uint8_t buf[] = { 0x02, 0x07, 0x00, 0x00 };
        uint16_t acc = (sd.accelSens * 60) + 3000;
        buf[2] = (uint8_t)acc;
        buf[3] = (uint8_t)(acc>>8);
        writeData(buf, 4);
    }

    if( bResubscribe )
        Subscribe();

    return true;
}

// sync read 
bool LevoReadWrite::ReadSync(enLevoBleDataType valueType, stBleVal& bleVal, uint32_t timeout )
{
    if (!IsConnected())
        return false;

    timeout += millis();
    if (ReadAsync(valueType, true))
    {
        // Serial.printf("Sync read 0x%04.4x\r\n", valueType );
        while (1)
        {
            if (Update(bleVal))
            {
                if (bleVal.dataType == valueType)
                    return true;
            }
            if (millis() > timeout)
                break;
            delay(1);
        }
    }
    return false;
}

// async read get result via Update();
bool LevoReadWrite::ReadAsync(enLevoBleDataType valueType, bool bForce )
{
    if (!IsConnected())
        return false;

    // start request if none is running yet
    if (m_requestedValueType == UNKNOWN || bForce)
    {
        // Serial.printf("Ble read request: 0x%02.2x 0x%02.2x\r\n", (uint8_t)valueType, (uint8_t)(valueType>>8));
        m_requestedValueType = valueType;
        RequestBleValue(m_requestedValueType);
        return true;
    }
    return false;
}

// query notication results and async read operations
bool LevoReadWrite::Update(stBleVal& bleVal)
{
    // get notification value from base class
    if( LevoEsp32Ble::Update( bleVal ) )
    {
        // dirty hack: after 0x02 0x27 notifications are paused for a short time so we can request the battery size
        if( bleVal.dataType == UNKNOWN && bleVal.raw.data[0] == 0x02 && bleVal.raw.data[1] == 0x27)
            ReadAsync(BATT_SIZEWH);

        return true;
    }

    // request is running, try to read a value
    if (m_requestedValueType != UNKNOWN)
    {
        if (ReadRequestedBleValue(m_requestedValueType, bleVal))
        {
            m_requestedValueType = UNKNOWN;
            return true;
        }
    }

    return false;
}
