/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
 *
 * logging data values to a file
 *
 */

#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include <LevoEsp32Ble.h>
#include "DisplayData.h"

class FileLogger
{
public:
    typedef enum
    {
        NONE = 0,
        SIMPLE,           // one line per value, no time- or distance-stamp 
        CSV_SIMPLE,       // one line per value, time an distance stamp, numeric id for value type
        CSV_KNOWN,        // one line per value, time an distance stamp, numeric id for value type, no UNKNOWN message
        CSV_KNOWNCHANGED, // one line per value, time an distance stamp, numeric id for value type, only changed values, no UNKNOWN message
        CSV_TABLE,        // all values per line with time- and distance-stamp, no UNKNOWN messages

        NUM_FORMATS   // number of format constants, must be at last position
    } enLogFormat;

    bool   Writeln(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, enLogFormat format, uint32_t timestamp );
    bool   Open();
    void   Close();
    void   Flush();
    int8_t PercentFull();

protected:
    uint32_t m_tiOpenFile = 0;
    uint32_t m_tiStart = 0;
    float    m_kmStart = 0.0f;
    float    m_kmLast  = 0.0f;
    bool     m_bFirstLine = false;

    bool  Writeln(const char* strLog);
    bool  Writeln(std::string& strLog);

    void  HexDump(char* pBuf, int nLen, std::string& str);
    void  LogDump(DisplayData::enIds id, float val, DisplayData& DispData, std::string& str);

    bool LogSimple(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, uint32_t timestamp);
    bool LogCsvSimple( DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, enLogFormat format, uint32_t timestamp);
    bool LogCsvTable(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, uint32_t timestamp);

    // for CSV_TABLE log and value change detection
    float m_tableLineBuffer[DisplayData::numElements] = {0};

    bool isStaticValue( const DisplayData::stDisplayData* pDesc )  { return (pDesc->flags & DisplayData::STATIC) ? true : false; }
    bool isDynamicValue( const DisplayData::stDisplayData* pDesc ) { return (pDesc->flags & DisplayData::DYNAMIC) ? true : false; }
    bool isTripValue( const DisplayData::stDisplayData* pDesc)     { return (pDesc->flags & DisplayData::TRIP) ? true : false; }

    bool hasChanged( float fVal1, float fVal2, int precision );
};

#endif // FILE_LOGGER_H