/** Levo BLE ESP32 Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Wok�ck
 *
 */

#include "DisplayData.h"

const DisplayData::stDisplayData * DisplayData::GetDescription(enIds id)
{
    if (id >= 0 && id < numElements)
        return &displayData[id];

    return NULL;
}
