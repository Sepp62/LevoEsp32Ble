/*
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
 *
 *  Rendering a data value field on M5 screen
 *
 */

#include <M5Core2.h>
#include "DisplayData.h"

#ifndef M5_FIELD_H
#define M5_FIELD_H

// for creating 565 colors (uint16_t) directly
// http://www.barth-dev.de/online/rgb565-color-picker/

#define LABEL_COLOR        35,255,35
#define VALUE_COLOR        255,255,255
#define UNIT_COLOR         130,130,130
#define FRAME_COLOR        124,163,121 // 130,130,130

#define BATTFILL_COLORGREEN    183,226,152
#define BATTFILL_COLORYELL     224,219,12
#define BATTFILL_COLORRED      222,13,34

class M5Field
{
protected:
    struct stColors
    {
        uint16_t colLabel;
        uint16_t colValue;
        uint16_t colUnit;
        uint16_t colFrame;
    };

    const stColors colors =
    {
        M5.Lcd.color565(LABEL_COLOR), // uint16_t colLabel;
        M5.Lcd.color565(VALUE_COLOR), // uint16_t colValue;
        M5.Lcd.color565(UNIT_COLOR), // uint16_t colUnit;
        M5.Lcd.color565(FRAME_COLOR), // uint16_t colFrame;
    };

    struct stMetrics
    {
        uint8_t txtHeightLabel;
        uint8_t txtHeightVal;
        uint16_t rcBoundWidth;
        uint16_t rcBoundHeight;
    };

    void printSpecialChar(const char* pStr);

public:
    typedef enum
    {
        NORMAL = 0,
        OFFLINE,
        // ALARM todo
    } enStyle;

    M5Field() {}

    virtual void RenderValue(char* strVal, int orgPointX, int orgPointY, const DisplayData::stDisplayData* pDesc, enStyle style );
    virtual void RenderFrame(int orgPointX, int orgPointY, const DisplayData::stDisplayData* pDesc);

    virtual const stMetrics& GetMetrics() = 0;
    virtual const stColors& GetColors() { return colors;  }
};


class M5FieldThird : public M5Field
{
protected:

    const stMetrics metrics =
    {
        8,                   // uint8_t txtHeightLabel;
        16,                  // uint8_t txtHeightVal;
        M5.Lcd.width() / 3 , // uint16_t rcBoundWidth;
        38,                  // uint16_t rcBoundHeight;
    };
public:
    virtual const stMetrics& GetMetrics() { return metrics; }
};


class M5FieldHalf : public M5Field
{
protected:

    const stMetrics metrics =
    {
        8,                   // uint8_t txtHeightLabel;
        16,                  // uint8_t txtHeightVal;
        M5.Lcd.width() / 2 , // uint16_t rcBoundWidth;
        38,                  // uint16_t rcBoundHeight;
    };
public:
    virtual const stMetrics& GetMetrics() { return metrics; }
};

#endif // M5_FIELD_H