/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 *  Setting RTC by NTP
 *
 */

#include "M5NTPTime.h"

I2C_BM8563 M5NTPTime::rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire1);

const char M5NTPTime::ntpServer[] = "time.cloudflare.com";

bool M5NTPTime::SetTime( const char* ssid, const char* wifiPwd )
{
    bool ret = false;

    Serial.println("Set RTC to NTP time...");

    // Connect to an access point
    WiFi.begin(ssid, wifiPwd);  // Or, Connect to the specified access point

    Serial.print("Connecting to Wi-Fi ");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" CONNECTED");

    // Set ntp time to local
    configTime(1 * 3600, 0, ntpServer);

    // Init I2C
    Wire1.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);

    // Init RTC
    rtc.begin();

    // Get local time
    struct tm timeInfo;
    if (getLocalTime(&timeInfo))
    {
        // Set RTC time
        I2C_BM8563_TimeTypeDef timeStruct;
        timeStruct.hours = timeInfo.tm_hour;
        timeStruct.minutes = timeInfo.tm_min;
        timeStruct.seconds = timeInfo.tm_sec;
        rtc.setTime(&timeStruct);

        // Set RTC Date
        I2C_BM8563_DateTypeDef dateStruct;
        dateStruct.weekDay = timeInfo.tm_wday;
        dateStruct.month = timeInfo.tm_mon + 1;
        dateStruct.date = timeInfo.tm_mday;
        dateStruct.year = timeInfo.tm_year + 1900;
        rtc.setDate(&dateStruct);

        // Print RTC
        Serial.printf("Date/Time: %04d/%02d/%02d %02d:%02d:%02d\n",
            dateStruct.year,
            dateStruct.month,
            dateStruct.date,
            timeStruct.hours,
            timeStruct.minutes,
            timeStruct.seconds
        );
        ret = true;
    }

    WiFi.disconnect(true, true);

    return ret;
}
