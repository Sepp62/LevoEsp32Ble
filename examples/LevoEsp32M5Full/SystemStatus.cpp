
// API Doc
// https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api
// https://lvgl.io/tools/imageconverter

#include "Arduino.h"

#include "SystemStatus.h"

// #include <Fonts/EVA_20px.h>
// #include <Fonts/EVA_11px.h>

void SystemStatus::UpdateBleStatus(LevoEsp32Ble::enBleStatus bleStatus)
{
    newBleStatus = bleStatus;
}

bool SystemStatus::IsNewBleStatus(LevoEsp32Ble::enBleStatus& bleStatus)
{
    if (lastBleStatus != newBleStatus)
    {
        bleStatus = newBleStatus;
        lastBleStatus = newBleStatus;
        return true;
    }
    return false;
}
