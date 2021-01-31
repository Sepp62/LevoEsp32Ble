// API Doc
// https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api

#include "Arduino.h"

#include "M5Screen.h"
#include "M5Field.h"

// bluetooth icons
extern const uint8_t btblue_map[];
extern const uint8_t btyellow_map[];
extern const uint8_t btgray_map[];
extern const uint8_t btred_map[];
extern const uint8_t img_battgreen_map[];
extern const uint8_t img_chargearrow_map[];

M5FieldThird thirdField;
M5FieldHalf  halfField;

void M5Screen::ShowValue(DisplayData::enIds id, float val, DisplayData& dispData)
{
    char strVal[20];

    const DisplayData::stDisplayData* pDesc = dispData.GetDescription(id);
    if (pDesc && pDesc->nWidth < sizeof( strVal ) )
    {
        dtostrf(val, pDesc->nWidth, pDesc->nPrecision, strVal );
        
        int idx = idToIdx[id];
        if (idx >= DisplayData::numElements)
            return;
        
        // render value
        stRender& render = fldRender[idx];
        render.pField->RenderValue(strVal, render.x, render.y, pDesc);
    }
}

void M5Screen::RenderEmptyValue(stRender& render, const DisplayData::stDisplayData* pDesc)
{
    char strVal[20];
    dtostrf( 0.0f, pDesc->nWidth, pDesc->nPrecision, strVal);
    render.pField->RenderValue( strVal, render.x, render.y, pDesc, M5Field::OFFLINE );
}

void M5Screen::Init(DisplayData& dispData)
{
    // build layout from id order 
    int i, x = 0, y = START_Y, bottom = 0;
    for (i = 0; i < DisplayData::numElements; i++)
    {
        // get data id
        DisplayData::enIds id = dispData.GetIdFromIdx(i);
        if (id == DisplayData::UNKNOWN)
            continue;
        
        // get data description
        const DisplayData::stDisplayData* pDesc = dispData.GetDescription( id );
        if (pDesc == 0)
            continue;

        // lookup for faster rendering of values
        idToIdx[id] = i;

        // needed field type
        if (pDesc->nWidth > 4)
            fldRender[i].pField = &halfField;
        else
            fldRender[i].pField = &thirdField;

        // field position
        fldRender[i].y = y;
        fldRender[i].x = x;

        // wrap line
        x += fldRender[i].pField->GetMetrics().rcBoundWidth;
        if( x > M5.Lcd.width() )
        {
            y += bottom;
            x = fldRender[i].pField->GetMetrics().rcBoundWidth;
            bottom = 0;
            fldRender[i].y = y;
            fldRender[i].x = 0;
        }

        // render frame and empty value
        fldRender[i].pField->RenderFrame(fldRender[i].x, fldRender[i].y, pDesc);
        RenderEmptyValue(fldRender[i], pDesc);

        // remember bottom of highest element 
        bottom = max(bottom, (int)fldRender[i].pField->GetMetrics().rcBoundHeight);
    }
}

bool M5Screen::ShowSysStatus(SystemStatus& sysStatus)
{
    bool ret = false;

    // Show BLE status
    LevoEsp32Ble::enBleStatus bleStatus;
    if (sysStatus.IsNewBleStatus(bleStatus))
    {
        switch (bleStatus)
        {
        case LevoEsp32Ble::CONNECTED:    M5.Lcd.drawBitmap(1, 1, 20, 26, btblue_map); break;
        case LevoEsp32Ble::CONNECTING:   M5.Lcd.drawBitmap(1, 1, 20, 26, btyellow_map); break;
        case LevoEsp32Ble::OFFLINE:      M5.Lcd.drawBitmap(1, 1, 20, 26, btgray_map); break;
        case LevoEsp32Ble::AUTHERROR:    M5.Lcd.drawBitmap(1, 1, 20, 26, btred_map);break;
        }
        ret = true;
    }

    // show battery status every 10 ticks (1000 ms)
    if ((showSysStatusCnt++ % 10) == 0)
    {
        // battery image with fill bar
        M5.Lcd.drawBitmap(32, 7, 30, 16, img_battgreen_map);  // 30x16 image
        int rectwidth = (int)(25.0 * sysStatus.batteryPercent/100.0);
        uint16_t battFillCol = M5.Lcd.color565(BATTFILL_COLORGREEN);
        if(sysStatus.batteryPercent < 20.0 )
            battFillCol = M5.Lcd.color565(BATTFILL_COLORRED);
        else if (sysStatus.batteryPercent < 50.0)
            battFillCol = M5.Lcd.color565(BATTFILL_COLORYELL);
        M5.Lcd.fillRect(34, 9, rectwidth, 12, battFillCol );

        // charge symbol
        int xpos = 68;
        M5.Lcd.fillRect(68, 7, 18, 16, BLACK);
        if (sysStatus.powerStatus != SystemStatus::POWER_INTERNAL)
        {
            M5.Lcd.drawBitmap(68, 7, 8, 16, img_chargearrow_map);  // 8x16 image
            xpos += 18;
        }

        // battery percent number
        M5.Lcd.setTextColor(M5.Lcd.color565(UNIT_COLOR));
        M5.Lcd.setTextSize(2);
        M5.Lcd.fillRect(xpos, 8, M5.Lcd.width() - xpos, 20, BLACK);
        M5.Lcd.setCursor(xpos, 8);
        M5.Lcd.printf("%d%%", sysStatus.batteryPercent);

        // clock
        char timeStrbuff[20];
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(218, 8);
        M5.Rtc.GetTime(&RTCtime_Now);
        sprintf(timeStrbuff, "%02d:%02d:%02d", RTCtime_Now.Hours, RTCtime_Now.Minutes, RTCtime_Now.Seconds);
        M5.Lcd.println(timeStrbuff);
    }
    return ret;
}
