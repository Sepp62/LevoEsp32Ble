/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 * management of peripheral components of M5
 *
*/

#ifndef M5SYSTEM_H
#define M5SYSTEM_H

#include <M5Core2.h>
#include "SystemStatus.h"

class M5System
{
public:
    void Init();
    void CheckPower(SystemStatus& sysStatus);
    void CheckSDCard(SystemStatus& sysStatus);
    void DoDisplayTimer();
    void ResetDisplayTimer(){ tiDisplayOff = 0; DoDisplayTimer();  }
    void SetBacklightSettings( uint16_t backlightTo, bool bBacklightChg ) { backlightTimeout = backlightTo; bBacklightCharging = bBacklightChg; }

protected:
    typedef struct i2cDevice
    {
        i2cDevice() {
            Name = "";
            addr = 0;
            nextPtr = nullptr;
        };
        String Name;
        uint8_t addr;
        struct i2cDevice* nextPtr;
    } i2cDevice_t;
    
    i2cDevice_t i2cParentptr;

    void addI2cDevice(String name, uint8_t addr);
    int  checkI2cAddr();
    void coverScrollText(String strNext, uint32_t color);
    void sysErrorSkip();

    uint32_t tiDisplayOff = 0L;

    // settings from peferences
    bool     bBacklightCharging = true;
    uint16_t backlightTimeout   = 60;
};

#endif // M5SYSTEM_H