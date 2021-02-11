// API Doc
// https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api

#include "M5Field.h"

// #include <Fonts/EVA_20px.h>
// #include <Fonts/EVA_11px.h>

void M5Field::printSpecialChar(const char* pStr)
{
    // degree symbol
    if (pStr[0] == 'o')
    {
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(M5.Lcd.getCursorX(), M5.Lcd.getCursorY() - 2 );
        M5.Lcd.print( "o" );
    }
    else
        M5.Lcd.print( pStr );
}

void M5Field::RenderValue(char * strVal, int orgPointX, int orgPointY, const DisplayData::stDisplayData* pDesc, enStyle style)
{
    // delete background
    int cx = GetMetrics().rcBoundWidth - 2;
    M5.Lcd.fillRect(orgPointX + 1, orgPointY + GetMetrics().txtHeightLabel + 7, cx, GetMetrics().txtHeightVal, BLACK);

    M5.Lcd.setTextFont(1);

    // print value
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor( (style == NORMAL) ? GetColors().colValue : GetColors().colUnit );
    M5.Lcd.setCursor(orgPointX + 3, orgPointY + GetMetrics().txtHeightLabel + 7);
    M5.Lcd.print(strVal);

    // print label
    M5.Lcd.setCursor(M5.Lcd.getCursorX() + 4, M5.Lcd.getCursorY());
    M5.Lcd.setTextColor(GetColors().colUnit);
    if (pDesc->strUnit[0] == '&')
        printSpecialChar( (pDesc->strUnit + 1 ) );
    else
        M5.Lcd.print(pDesc->strUnit);
}

void M5Field::RenderFrame(int orgPointX, int orgPointY, const DisplayData::stDisplayData* pDesc)
{
    // frame rect
    M5.Lcd.drawRect( orgPointX, orgPointY, GetMetrics().rcBoundWidth, GetMetrics().rcBoundHeight, GetColors().colFrame );
    // label
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextColor(GetColors().colLabel);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(orgPointX + 3, orgPointY + 2 );
    M5.Lcd.print(String(pDesc->strLabel));
}
