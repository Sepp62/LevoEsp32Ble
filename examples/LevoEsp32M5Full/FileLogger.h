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
        SIMPLE,       // one line per value, no time- or distance-stamp 
        CSV_SIMPLE,   // one line per value, time an distance stamp, numeric id for value type
        CSV_KNOWN,    // one line per value, time an distance stamp, numeric id for value type, no UNKNOWN message
        CSV_TABLE,    // all values per line with time- and distance-stamp, no UNKNOWN messages

        NUM_FORMATS   // number of format constants, must be at last position
    } enLogFormat;

    bool   Writeln(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, enLogFormat format = SIMPLE);
    bool   Open();
    void   Close();
    int8_t PercentFull();

protected:
    uint32_t tiStart = 0;
    float    kmStart = 0.0f;
    float    kmLast  = 0.0f;
    bool     bFirstLine = false;

    bool  Writeln(const char* strLog);

    void  HexDump(char* pBuf, int nLen, std::string& str);
    void  LogDump(DisplayData::enIds id, float val, DisplayData& DispData, std::string& str);

    bool LogSimple(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData);
    bool LogCsvSimple( DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData, enLogFormat format);
    bool LogCsvTable(DisplayData::enIds id, LevoEsp32Ble::stBleVal& bleVal, DisplayData& DispData);

    // for CSV_TABLE log
    float allValues[DisplayData::numElements];
};

#endif // FILE_LOGGER_H