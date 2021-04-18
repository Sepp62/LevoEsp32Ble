/*
 *
 *  Created: 21/03/2021
 *      Author: Bernd Wokoeck
 *
 * deliver IMU sensor data
 *
 */

// todo:
// - needs calibration to +90° and -90°
// - needs calibration to 0° according to mounting position on bike with help of baro inclination
// - 0,5% is 0.29° --> will this sensor be too noisy for this ? May be baro inclination will be better :-(

#include "IMUSensors.h"

bool IMUSensors::Init()
{
    if (M5.IMU.Init() == 0)
    {
        M5.IMU.SetGyroFsr(MPU6886::GFS_250DPS);
        return true;
    }
    return false;
}

void IMUSensors::FeedValue(DisplayData::enIds id, float fVal, uint32_t timestamp)
{
    // barometric inclination value will calibrate IMU
    if (id == DisplayData::VIRT_INCLINATION)
    {

    }
}

bool IMUSensors::Update(DisplayData::enIds& id, float& fVal, uint32_t timestamp)
{
    float pitch, roll, yaw;
    M5.IMU.getAhrsData(&pitch, &roll, &yaw);
    fVal = roll;
    id = DisplayData::GYRO_PITCH;
    return true;
}

