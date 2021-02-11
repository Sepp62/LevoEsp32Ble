/** Levo BLE ESP32 Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 */

#include "DisplayData.h"

DisplayData::enIds DisplayData::GetIdFromIdx(int i)
{
    if (i < numElements)
        return order[i];
    else
        return UNKNOWN;
}

const DisplayData::stDisplayData * DisplayData::GetDescription(enIds id)
{
    if (id >= 0 && id < numElements)
        return &displayData[id];

    return NULL;
}
