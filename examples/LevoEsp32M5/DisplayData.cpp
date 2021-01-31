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
    if (id < numElements)
        return &displayData[id];

    return 0;
}

// format binary data to hex 
void DisplayData::HexDump(char* pBuf, int nLen, std::string& str )
{
    char s[] = "00 ";
    for (int i = 0; i < nLen; i++)
    {
        snprintf(s, sizeof(s), "%02x ", pBuf[i]);
        str += s;
    }
}

// format for log output
void DisplayData::LogDump(enIds id, float val, std::string& str )
{
    const stDisplayData* pDesc = GetDescription(id);
    if ( pDesc == 0 )
        return;

    // format float
    char strVal[20];
    dtostrf(val, 7, pDesc->nPrecision, strVal);
    // format line
    char s[80];
    snprintf(s, sizeof(s), "%-14s%s %s", pDesc->strLabel, strVal, pDesc->strUnit);
    // replace degree symbol
    char* p = strstr(s, "&o");
    if (p)
    {
        *p = 0xB0;
        *(p + 1) = ' ';
    }
    str = s;
}
