/** Levo BLE ESP32 Data Dumper
 *
 *  Created: 01/01/2021
 *      Author: Bernd Woköck
 *
 */

#include "DisplayData.h"

const DisplayData::stDisplayData * DisplayData::GetDescription(enIds id)
{
    if( hiddenMask.test( id ) ) // skip hidden 
        return NULL;

    if (id >= 0 && id < numElements)
        return &displayData[id];

    return NULL;
}
