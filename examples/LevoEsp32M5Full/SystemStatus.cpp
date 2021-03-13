/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
 *
 * management of system M5 status
 *
 *
*/

#include "Arduino.h"

#include "SystemStatus.h"

// #include <Fonts/EVA_20px.h>
// #include <Fonts/EVA_11px.h>


// reset status and force BT icon to be redrawn
void SystemStatus::ResetBleStatus(LevoEsp32Ble::enBleStatus status )
{
    newBleStatus = status;
    lastBleStatus = LevoEsp32Ble::UNDEFINED;
}

// set new ble status 
void SystemStatus::UpdateBleStatus(LevoEsp32Ble::enBleStatus bleStatus)
{
    newBleStatus = bleStatus;
}

// check if status has changed
bool SystemStatus::IsNewBleStatus(LevoEsp32Ble::enBleStatus& bleStatus)
{
    if (lastBleStatus != newBleStatus)
    {
        Serial.printf( "ble status change: %d --> %d\r\n", lastBleStatus, newBleStatus);
        bleStatus = newBleStatus;
        lastBleStatus = newBleStatus;
        return true;
    }
    return false;
}
