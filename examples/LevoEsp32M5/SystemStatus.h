#include <M5Core2.h>
#include "LevoEsp32Ble.h"

#ifndef SYSTEMSTATUS_H
#define SYSTEMSTATUS_H

class SystemStatus
{
protected:
    LevoEsp32Ble::enBleStatus newBleStatus = LevoEsp32Ble::OFFLINE;
    LevoEsp32Ble::enBleStatus lastBleStatus = LevoEsp32Ble::UNDEFINED;

public:
    typedef enum
    {
        POWER_EXTERNAL = 0,
        POWER_INTERNAL,
        POWER_MAX
    } enSysPower;

    float      batterVoltage   = 0.0f;
    int        batteryPercent  = 0;
    enSysPower powerStatus     = POWER_INTERNAL;

    void UpdateBleStatus( LevoEsp32Ble::enBleStatus bleStatus );
    bool IsNewBleStatus(LevoEsp32Ble::enBleStatus& bleStatus);
};

#endif // SYSTEMSTATUS_H