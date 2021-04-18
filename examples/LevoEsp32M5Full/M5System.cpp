/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
 *
 * management of peripheral components of M5
 *
 * API Doc
 * https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api
 *
*/

#include "M5System.h"

#include <Fonts/EVA_20px.h>
#include <Fonts/EVA_11px.h>

TFT_eSprite Disbuff = TFT_eSprite(&M5.Lcd);
TFT_eSprite DisCoverScrollbuff = TFT_eSprite(&M5.Lcd);

#define FAILED_COLOR     255,35,35
#define SUCCESS_COLOR    255,255,255

// switch lcd backlight off after some time to save battery power
void M5System::DoDisplayTimer()
{
    bool bKeepOn = M5.Axp.isACIN() && bBacklightCharging;

    // see https://github.com/m5stack/M5Core2/blob/master/src/AXP192.cpp
    if( tiDisplayOff == 0 || M5.Touch.ispressed() || bKeepOn )
    {
        if( tiDisplayOff == UINT32_MAX )
        {
            Serial.println( "turn backlight on");
            M5.Axp.SetLed(0);
            M5.Axp.SetDCDC3(true);  // lcd backlight light on
        }
        tiDisplayOff = millis() + (uint32_t)backlightTimeout * 1000L;
    }

    // check timeout
    if (millis() > tiDisplayOff )
    {
        // Serial.println(tiDisplayOff);
        M5.Axp.SetDCDC3(false); // lcd backlight light off
        M5.Axp.SetLed( 1 );     // indicate unit is running
        tiDisplayOff = UINT32_MAX;
    }

}

void M5System::addI2cDevice(String name, uint8_t addr )
{
    i2cDevice_t *lastptr = &i2cParentptr;
    
    while(lastptr->nextPtr != nullptr )
    {
        lastptr = lastptr->nextPtr;
    }

    i2cDevice_t* ptr = ( i2cDevice_t * )calloc(1,sizeof(i2cDevice_t));
    ptr->Name = name;
    ptr->addr = addr;
    ptr->nextPtr = nullptr;
    lastptr->nextPtr = ptr;
}

void M5System::sysErrorSkip()
{
    uint32_t bkColor16 = Disbuff.color565(0x22, 0x22, 0x22);
    Disbuff.setFreeFont(&EVA_20px);
    Disbuff.setTextSize(1);
    Disbuff.setTextColor(Disbuff.color565(0xff, 0, 0), bkColor16);
    Disbuff.setTextDatum(TC_DATUM);
    Disbuff.pushInSprite(&DisCoverScrollbuff, 0, 0, 320, 60, 0, 150);
    Disbuff.setCursor(94, 220);
    Disbuff.print("Touch to Skip");
    Disbuff.pushSprite(0, 0);

    HotZone_t touchZone(0, 0, 320, 280);

    while (1)
    {
        if (M5.Touch.ispressed())
        {
            TouchPoint_t point = M5.Touch.getPressPoint();
            if (touchZone.inHotZone(point))
                break;
        }
        delay(10);
    }
}

void M5System::coverScrollText(String strNext, uint32_t color)
{
    static String strLast;
    uint32_t colorLast = 0xffff;
    uint32_t bkColor16 = DisCoverScrollbuff.color565(0x22, 0x22, 0x22);
    DisCoverScrollbuff.setFreeFont(&EVA_20px);
    DisCoverScrollbuff.setTextSize(1);
    DisCoverScrollbuff.setTextColor(Disbuff.color565(255, 0, 0), bkColor16);
    DisCoverScrollbuff.setTextDatum(TC_DATUM);
    DisCoverScrollbuff.fillRect(0, 0, 320, 60, bkColor16);
    DisCoverScrollbuff.setTextColor(color);
    for (int i = 0; i < 20; i++)
    {
        DisCoverScrollbuff.fillRect(0, 20, 320, 20, bkColor16);
        DisCoverScrollbuff.setTextColor(colorLast);
        DisCoverScrollbuff.drawString(strLast, 160, 20 - i);
        DisCoverScrollbuff.setTextColor(color);
        DisCoverScrollbuff.drawString(strNext, 160, 40 - i);
        DisCoverScrollbuff.fillRect(0, 0, 320, 20, bkColor16);
        DisCoverScrollbuff.fillRect(0, 40, 320, 20, bkColor16);
        delay(5);
        DisCoverScrollbuff.pushSprite(0, 150);
    }
    strLast = strNext;
    colorLast = color;
}

int M5System::checkI2cAddr()
{
    uint8_t count = 0;
    i2cDevice_t* lastptr = &i2cParentptr;
    do {
        lastptr = lastptr->nextPtr;
        Serial.printf("Addr:0x%02X - Name:%s\r\n", lastptr->addr, lastptr->Name.c_str());
        Wire1.beginTransmission(lastptr->addr);
        if (Wire1.endTransmission() == 0)
        {
            String log = "I2C " + lastptr->Name + " Found";
            coverScrollText(log, M5.Lcd.color565(SUCCESS_COLOR));
        }
        else
        {
            String log = "I2C " + lastptr->Name + " Find failed";
            coverScrollText(log, M5.Lcd.color565(FAILED_COLOR));
            sysErrorSkip();
        }
        delay(100);
        count++;
    } while (lastptr->nextPtr != nullptr);
    return 0;
}

void M5System::CheckPowerSupply(SystemStatus& sysStatus)
{
    sysStatus.powerStatus = ( M5.Axp.isACIN()) ? SystemStatus::POWER_EXTERNAL : SystemStatus::POWER_INTERNAL;
    sysStatus.batterVoltage = M5.Axp.GetBatVoltage();
    sysStatus.batteryPercent = (sysStatus.batterVoltage < 3.2f ) ? 0 : (sysStatus.batterVoltage - 3.2f ) * 100;
}

void M5System::CheckSDCard(SystemStatus& sysStatus)
{
    sdcard_type_t Type = SD.cardType();
    if (Type == CARD_UNKNOWN || Type == CARD_NONE)
        sysStatus.bHasSDCard = false;
    else
        sysStatus.bHasSDCard = true;
}

void M5System::Init()
{
    M5.begin(true, true, true, true);
    M5.Lcd.fillScreen(BLACK);

    M5.Axp.SetLed(1);
    delay(100);
    M5.Axp.SetLed(0);

    Disbuff.createSprite(320, 240);

    // Wire1 devices
    addI2cDevice("Axp192",0x34);
    addI2cDevice("CST Touch",0x38);
    addI2cDevice("IMU6886",0x68);
    addI2cDevice("BM8563",0x51);

    M5.Axp.SetLcdVoltage(3300);
    SD.begin();
    M5.Axp.SetBusPowerMode(0);
    M5.Axp.SetCHGCurrent(AXP192::kCHG_190mA);
    M5.Axp.SetSpkEnable( false );
    M5.Axp.SetLDOEnable(3,false); 
    M5.Axp.SetLDOVoltage(3, 3300);

    DisCoverScrollbuff.createSprite(320, 60);
    checkI2cAddr();
    coverScrollText("LEVO BLE Version 0.99", M5.Lcd.color565(SUCCESS_COLOR));
    DisCoverScrollbuff.deleteSprite();

    delay(500);
    M5.Lcd.fillScreen(BLACK);
}