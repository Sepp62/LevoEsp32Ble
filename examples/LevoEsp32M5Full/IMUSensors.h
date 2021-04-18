/*
 *
 *  Created: 21/03/2021
 *      Author: Bernd Wokoeck
 *
 * deliver IMU sensor data
 *
 */

#ifndef IMUSENSORS_H
#define IMUSENSORS_H

#include <M5Core2.h>
#include "SystemStatus.h"
#include "DisplayData.h"

class IMUSensors
{
public:
    bool Init();

    void FeedValue(DisplayData::enIds id, float fVal, uint32_t timestamp); // value from any other sensor
    bool Update(DisplayData::enIds& id, float& fVal, uint32_t timestamp);  // poll IMU sensor values

protected:
};

#endif // IMUSENSORS_H