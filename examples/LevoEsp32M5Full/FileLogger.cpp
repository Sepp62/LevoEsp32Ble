/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 * logging data values to a file
 *
 */

#include <M5Core2.h>
#include "FileLogger.h"

File dataFile;

// check SD card occupied space in %
int8_t FileLogger::PercentFull()
{
    // check card
    sdcard_type_t Type = SD.cardType();
    if (Type == CARD_UNKNOWN || Type == CARD_NONE)
    {
        // Serial.println("No SD card.");
        return -1;
    }

    // used percent
    uint64_t used = SD.usedBytes();
    if (used == 0)
        used = (uint64_t)1;
    uint64_t usedPercent = (used * (uint64_t)100) / SD.totalBytes();

    return (int8_t)usedPercent;
}

// try to open log file
bool FileLogger::Open()
{
    // reset timestamp and start distance
    tiOpenFile = millis();
    tiStart = 0;
    kmStart = 0.0f;
    kmLast = 0.0f;
    bFirstLine = true;

    // reset line buffer
    for( int i = 0; i < DisplayData::numElements; i++ )
     allValues[i] = 0.0f;

    // check card
    sdcard_type_t Type = SD.cardType();
    if (Type == CARD_UNKNOWN || Type == CARD_NONE)
    {
        Serial.println("No SD card.");
        return false;
    }

    // free space
    uint64_t freeBytes = (SD.totalBytes() - SD.usedBytes()) / (uint64_t)1000;
    Serial.print("SD card free: ");
    Serial.print( (uint32_t)freeBytes );
    Serial.println(" KB");

    // used percent
    Serial.print("SD percent used: ");
    Serial.print((int)PercentFull());
    Serial.println(" %");

    // open file in append mode
    dataFile = SD.open("/levolog.txt", FILE_APPEND);
    if (!dataFile)
    {
        Serial.println("Error opening log file.");
        return false;
    }

    // log date time
    RTC_TimeTypeDef RTCtime;
    RTC_DateTypeDef RTC_Date;
    char timeStrbuff[40];
    M5.Rtc.GetDate(&RTC_Date);
    M5.Rtc.GetTime(&RTCtime);
    sprintf(timeStrbuff, "%02d.%02d.%04d, %02d:%02d:%02d", RTC_Date.Date, RTC_Date.Month, RTC_Date.Year, RTCtime.Hours, RTCtime.Minutes, RTCtime.Seconds);
    Writeln(timeStrbuff);
    Writeln("====================");

    return true;
}

void FileLogger::Close()
{
    dataFile.close();
    Serial.println("log file closed.");
}

// write a text line to file
bool FileLogger::Writeln(const char* strLog)
{
    if (dataFile)
    {
        dataFile.println(strLog);
        return true;
    }
    return false;
}

// log one line in the specified format
bool FileLogger::Writeln(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, enLogFormat format)
{
    // set start values for time
    if( bFirstLine )
        tiStart = millis();

    // set start values for distance
    if( kmStart == 0.0f && id == DisplayData::BLE_MOT_ODOMETER && bleVal.unionType == LevoEsp32Ble::FLOAT )
        kmStart = bleVal.fVal;

    // remember last distance value
    if (id == DisplayData::BLE_MOT_ODOMETER && bleVal.unionType == LevoEsp32Ble::FLOAT)
    {
        kmLast = bleVal.fVal;
    }

    // various log formats
    switch (format)
    {
    case SIMPLE:
        return LogSimple(id, bleVal, DispData);

    case CSV_SIMPLE:
    case CSV_KNOWN:
        return LogCsvSimple( id, bleVal, DispData, format);

    case CSV_TABLE:
        return LogCsvTable(id, bleVal, DispData);
    }
    return false;
}

// format binary data to hex 
void FileLogger::HexDump(char* pBuf, int nLen, std::string& str)
{
    char s[] = "00 ";
    for (int i = 0; i < nLen; i++)
    {
        snprintf(s, sizeof(s), "%02x ", pBuf[i]);
        str += s;
    }
}

// format for log output
void FileLogger::LogDump(DisplayData::enIds id, float val, DisplayData& DispData, std::string& str)
{
    const DisplayData::stDisplayData* pDesc = DispData.GetDescription(id);
    if (pDesc == 0)
        return;

    // format float value
    char strVal[20];
    dtostrf(val, 7, pDesc->nPrecision, strVal);

    // format line
    char s[80];
    snprintf(s, sizeof(s), "%-14s%s %s", pDesc->strLabel, strVal, pDesc->strUnit);

    // replace degree symbol
    char* p = strstr(s, "&o");
    if (p)
    {
        *p = 0xB0;
        *(p + 1) = ' ';
    }
    str = s;
}

// log value in text or hex representation w/o timestamp
bool FileLogger::LogSimple(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData)
{
    std::string strLog;

    // log header
    if(bFirstLine)
    {
        strLog = "Label         Value";
        Writeln(strLog.c_str());
        Serial.println(strLog.c_str());
        bFirstLine = false;
    }

    // log known data 
    if (id != DisplayData::UNKNOWN && bleVal.unionType == LevoEsp32Ble::FLOAT)
    {
        LogDump(id, bleVal.fVal, DispData, strLog);       // format string for simple log output
    }
    // log unknown data
    else if (bleVal.unionType == LevoEsp32Ble::BINARY)   // undecoded/raw for hex byte log output
    {
        strLog = "UNKNOWN\t";
        HexDump((char*)bleVal.raw.data, bleVal.raw.len, strLog);
    }
    Serial.println(strLog.c_str());
    return Writeln(strLog.c_str());
}

// log tab separated with timestamp and distance 
bool FileLogger::LogCsvSimple(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, enLogFormat format)
{
    // log buffer
    char strLog[200] = "";

    // also log unknown?
    bool bLogUnknown = (format == CSV_KNOWN) ? false : true;

    // write header to csv
    if (bFirstLine)
    {
        strncpy( strLog, "Time\tDist\tId\tLabel\tValue\tUnit", sizeof( strLog) );
        Writeln( strLog );
        Serial.println(strLog);
        bFirstLine = false;
    }

    // timestamp string
    char strTime[20];
    uint32_t ti = (millis() - tiStart) / 100; // in 1/10 seconds
    snprintf(strTime, sizeof(strTime), "%ld.%ld", ti / 10, ti % 10);

    // distance string
    char strDistance[20];
    dtostrf(kmLast - kmStart, 7, 2, strDistance);

    // log known value
    if (id != DisplayData::UNKNOWN && bleVal.unionType == LevoEsp32Ble::FLOAT)
    {
        const DisplayData::stDisplayData* pDesc = DispData.GetDescription(id);
        if (pDesc == 0)
            return false;

        // format float value with correct precision
        char strVal[20];
        dtostrf(bleVal.fVal, 7, pDesc->nPrecision, strVal);

        // replace degree symbol
        char strUnit[10];
        strncpy( strUnit, pDesc->strUnit, sizeof(strUnit) );
        if( strUnit[0] == '&' && strUnit[1] == 'o' )
        {
            strUnit[0] = '\xb0';
            strUnit[1] = '\0';
        }

        // log to file
        // time  km  id  label  value  unit
        snprintf(strLog, sizeof(strLog), "%s\t%s\t%d\t%s\t%s\t%s", strTime, strDistance, id, pDesc->strLabel, strVal, strUnit );
        Serial.println(strLog);
        return Writeln( strLog );
    }
    // log unknown data
    else if (bLogUnknown && id == DisplayData::UNKNOWN && bleVal.unionType == LevoEsp32Ble::BINARY)
    {
        // time  km -1 hexdump
        std::string strHex;
        HexDump((char*)bleVal.raw.data, bleVal.raw.len, strHex);
        snprintf(strLog, sizeof(strLog), "%s\t%s\t-1\t%s", strTime, strDistance, strHex.c_str());
        Serial.println(strLog);
        return Writeln(strLog);
    }

    return false;
}

bool FileLogger::LogCsvTable(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData)
{
    int i = 0;
    std::string strLog;

    // we can only handle float values
    if (id == DisplayData::UNKNOWN && bleVal.unionType != LevoEsp32Ble::FLOAT)
        return false;

    // remember value
    allValues[id] = bleVal.fVal;

    // write header to csv
    if (bFirstLine)
    {
        // collect data for 5 seconds
        uint32_t tiWait  = (millis() - tiOpenFile) / 1000L;
        if( tiWait < 5 )
            return true;

        // show static settings (hopefully collected within the first 10 seconds)
        for (i = 0; i < DisplayData::numElements; i++)
        {
            if (isStaticData((DisplayData::enIds)i))
            {
                float value = allValues[(DisplayData::enIds)i];
                LogDump((DisplayData::enIds)i, value, DispData, strLog);
                Writeln(strLog.c_str());
                Serial.println(strLog.c_str());
            }
        }

        // head line
        strLog = "Time\tDist\tId";
        for ( i = 0; i < DisplayData::numElements; i++)
        {
            if (isStaticData((DisplayData::enIds)i))
                continue;

            strLog += "\t";
            const DisplayData::stDisplayData* pDesc = DispData.GetDescription((DisplayData::enIds)i);
            if( pDesc )
                strLog += pDesc->strLabel;
        }
        Writeln(strLog.c_str());
        Serial.println(strLog.c_str());
        bFirstLine = false;
    }

    // timestamp string
    char strTime[20];
    uint32_t ti = (millis() - tiStart) / 100; // in 1/10 seconds
    snprintf(strTime, sizeof(strTime), "%ld.%ld", ti / 10, ti % 10);

    // distance string
    char strDistance[20];
    dtostrf(kmLast - kmStart, 7, 2, strDistance);

    // id string
    char strId[5];
    snprintf(strId, sizeof(strId), "%d", id);

    // build line
    strLog = strTime;
    strLog += "\t";
    strLog += strDistance;
    strLog += "\t";
    strLog += strId;

    // append all values
    for ( i = 0; i < DisplayData::numElements; i++)
    {
        if(isStaticData((DisplayData::enIds)i ) )
            continue;

        strLog += "\t";

        const DisplayData::stDisplayData* pDesc = DispData.GetDescription((DisplayData::enIds)i);
        if( pDesc )
        {
            // format float value with correct precision
            char strVal[20];
            dtostrf(allValues[i], 7, pDesc->nPrecision, strVal);
            strLog += strVal;
        }
    }

    Serial.println(strLog.c_str());
    return Writeln(strLog.c_str());
}

