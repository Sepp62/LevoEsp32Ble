#include <M5Core2.h>
#include "FileLogger.h"

File dataFile;

bool FileLogger::Open()
{
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

bool FileLogger::Writeln(const char* strLog)
{
    if (dataFile)
    {
        dataFile.println(strLog);
        return true;
    }
    return false;
}

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
