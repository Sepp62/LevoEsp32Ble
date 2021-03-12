/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 * management of system M5 status
 *
*/

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
    typedef enum
    {
        NONE = 0,
        STARTED,
        STOPPED,
    } enTripStatus;

    float        batterVoltage      = 0.0f;
    int          batteryPercent     = 0;
    enSysPower   powerStatus        = POWER_INTERNAL;
    bool         bHasBtPin          = false; // indicate if bluetooth pin is configured
    bool         bLogging           = false;
    bool         bHasSDCard         = false;
    enTripStatus tripStatus       = NONE;

    void UpdateBleStatus( LevoEsp32Ble::enBleStatus bleStatus );
    bool IsNewBleStatus(LevoEsp32Ble::enBleStatus& bleStatus);
    void ResetBleStatus(LevoEsp32Ble::enBleStatus status = LevoEsp32Ble::OFFLINE);
};

#endif // SYSTEMSTATUS_H