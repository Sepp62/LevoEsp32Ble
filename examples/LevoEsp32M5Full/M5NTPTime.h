#ifndef M5NTPTIME_H
#define M5NTPTIME_H

#include <M5Core2.h>
#include <I2C_BM8563.h>
#include <WiFi.h>

class M5NTPTime
{
public:
    bool SetTime( const char * ssid, const char * wifiPwd );

protected:

    // RTC BM8563 I2C port
    // I2C pin definition for M5Stick & M5Stick Plus & M5Stack Core2
    const int BM8563_I2C_SDA = 21;
    const int BM8563_I2C_SCL = 22;

    static const char ntpServer[];
    static I2C_BM8563 rtc;
};

#endif // M5NTPTIME_H