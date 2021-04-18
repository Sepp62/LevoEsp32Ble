/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
 *
 * main entry point for display of all data values on M5
 * 
 * API Doc
 * https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api
 *
 */

#include "Arduino.h"

#include "M5Screen.h"
#include "M5Field.h"
#include "M5ConfigForms.h"

// icons
extern const uint8_t btblue_map[];
extern const uint8_t btyellow_map[];
extern const uint8_t btgray_map[];
extern const uint8_t btred_map[];
extern const uint8_t btblack_map[];
extern const uint8_t img_battgreen_map[];
extern const uint8_t img_chargearrow_map[];
extern const uint8_t img_settings_map[];
extern const uint8_t img_sdgreen_map[];
extern const uint8_t img_sdgray_map[];
extern const uint8_t img_triprun_map[];
extern const uint8_t img_tripstop_map[];

M5FieldThird thirdField;
M5FieldHalf  halfField;

// val is in seconds
void M5Screen::formatAsTime(float val, size_t nLen, char* strVal)
{
    int h, m, s;
    h = val/3600.0;
    m = (val - h*3600.0)/60.0;
    s = val - h*3600.0 - m * 60.0;
    snprintf( strVal, nLen, " %02.2d:%02.2d:%02.2d", h, m, s );
}

void M5Screen::ShowValue( DisplayData::enIds id, float val, DisplayData& dispData )
{
    char strVal[20];

    // buffer value in case of screen change
    if( id < DisplayData::NUM_ELEMENTS )
      m_valueBuffer[id] = val;

    const DisplayData::stDisplayData* pDesc = dispData.GetDescription(id);
    if (pDesc && pDesc->nWidth < sizeof( strVal ) )
    {
        if( pDesc->flags & DisplayData::TIME )
            formatAsTime( val, sizeof(strVal), strVal );
        else
            dtostrf(val, pDesc->nWidth, pDesc->nPrecision, strVal );
        
        int idx = m_idToIdx[id];
        if (idx < 0 || idx >= DisplayData::NUM_ELEMENTS)
            return;

        // style/text color
        M5Field::enStyle style = M5Field::NORMAL;
        if( pDesc->flags & DisplayData::TRIP && m_sysStatus.tripStatus != SystemStatus::STARTED )
            style = M5Field::OFFLINE;
        
        // render value
        stRender& render = m_fldRender[idx];
        render.pField->RenderValue(strVal, render.x, render.y, pDesc, style);
    }
}

void M5Screen::renderEmptyValue(stRender& render, const DisplayData::stDisplayData* pDesc)
{
    char strVal[20];
    if (pDesc->flags & DisplayData::TIME)
        formatAsTime(0.0, sizeof(strVal), strVal);
    else
        dtostrf( 0.0f, pDesc->nWidth, pDesc->nPrecision, strVal);
    render.pField->RenderValue( strVal, render.x, render.y, pDesc, M5Field::OFFLINE );
}

DisplayData::enIds M5Screen::getIdFromIdx(enScreens nScreen, int i)
{
    if (i < DisplayData::numElements)
    {
        if(m_order[nScreen][i] != UNKNOWN )
            return (DisplayData::enIds)(m_order[nScreen][i] & 0x7FFF);
    }
    return DisplayData::UNKNOWN;
}

bool M5Screen::hasLineBreak(enScreens nScreen, int i)
{
    if (i < DisplayData::NUM_ELEMENTS)
    {
        if (m_order[nScreen][i] != UNKNOWN)
            return (m_order[nScreen][i] & NEWLINE ) ? true : false;
    }
    return false;
}

void M5Screen::Init(enScreens nScreen, DisplayData& dispData)
{
    M5.Lcd.clear();

    // Hardware buttons
    UpdateHardwareButtons(nScreen);

    // Button bar 
    enableButtonBar((nScreen == SCREEN_C) ? m_fnButtonBarEvent : NULL);

    // BT icon
    drawBluetoothIcon(m_lastBleStatus);

    // settings icon
    M5.Lcd.drawBitmap(M5.Lcd.width() - 26, 5, 20, 20, img_settings_map);  // 20x20 image

    // reset lookup table
    memset(m_idToIdx, -1, sizeof(m_idToIdx));

    // build layout from id order 
    int i, x = 0, y = START_Y, bottom = 0;
    for (i = 0; i < DisplayData::numElements; i++)
    {
        // get data id
        DisplayData::enIds id = getIdFromIdx(nScreen, i);
        if (id == DisplayData::UNKNOWN) // value not to be displayed on current screen
            continue;
        
        // get data description
        const DisplayData::stDisplayData* pDesc = dispData.GetDescription( id );
        if (pDesc == 0)
            continue;

        // lookup for faster rendering of values
        m_idToIdx[id] = i;

        // needed field width 
        if (pDesc->nWidth > 4)
            m_fldRender[i].pField = &halfField;
        else
            m_fldRender[i].pField = &thirdField;

        // field position
        m_fldRender[i].y = y;
        m_fldRender[i].x = x;

        // wrap line
        x += m_fldRender[i].pField->GetMetrics().rcBoundWidth;
        if (x > M5.Lcd.width() || hasLineBreak(nScreen, i))
        {
            y += bottom;
            x = m_fldRender[i].pField->GetMetrics().rcBoundWidth;
            bottom = 0;
            m_fldRender[i].y = y;
            m_fldRender[i].x = 0;
        }

        // render frame and buffered or empty value
        m_fldRender[i].pField->RenderFrame(m_fldRender[i].x, m_fldRender[i].y, pDesc);
        if(m_valueBuffer[id] != FLOAT_UNDEFINED )
            ShowValue( id, m_valueBuffer[id], dispData );
        else
            renderEmptyValue(m_fldRender[i], pDesc);

        // remember bottom of highest element 
        bottom = max(bottom, (int)m_fldRender[i].pField->GetMetrics().rcBoundHeight);
    }

    m_showSysStatusCnt = 0;
}

void M5Screen::UpdateHardwareButtons(enScreens nScreen)
{
    M5.Lcd.setFreeFont(FF1);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setTextColor((nScreen == SCREEN_A) ? TFT_WHITE : TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("Data A", 55, 224, 2);
    M5.Lcd.setTextColor((nScreen == SCREEN_B) ? TFT_WHITE : TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("Data B", 160, 224, 2);
    M5.Lcd.setTextColor((nScreen == SCREEN_C) ? TFT_WHITE : TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("Trip & Tune", 265, 224, 2);
}

void M5Screen::drawBluetoothIcon(LevoEsp32Ble::enBleStatus bleStatus)
{
    switch (bleStatus)
    {
    case LevoEsp32Ble::SWITCHEDOFF:  M5.Lcd.drawBitmap(1, 1, 20, 26, btblack_map); break;
    case LevoEsp32Ble::CONNECTED:    M5.Lcd.drawBitmap(1, 1, 20, 26, btblue_map); break;
    case LevoEsp32Ble::CONNECTING:   M5.Lcd.drawBitmap(1, 1, 20, 26, btyellow_map); break;
    case LevoEsp32Ble::OFFLINE:      M5.Lcd.drawBitmap(1, 1, 20, 26, btgray_map); break;
    case LevoEsp32Ble::AUTHERROR:    M5.Lcd.drawBitmap(1, 1, 20, 26, btred_map); break;
    }
}

void M5Screen::updateTripButtonStatus()
{
    if (m_sysStatus.tripStatus == SystemStatus::STARTED)
    {
        SetButtonBarButtonState(BTSTART, true);
        SetButtonBarButtonState(BTSTOP, false);
    }
    else if (m_sysStatus.tripStatus == SystemStatus::STOPPED)
    {
        SetButtonBarButtonState(BTSTART, false);
        SetButtonBarButtonState(BTSTOP, true);
    }
    else if (m_sysStatus.tripStatus == SystemStatus::NONE)
    {
        SetButtonBarButtonState(BTSTART, false);
        SetButtonBarButtonState(BTSTOP, false);
    }
}

bool M5Screen::ShowSysStatus()
{
    bool ret = false;

    // Show BLE status
    if (m_sysStatus.IsNewBleStatus(m_lastBleStatus) )
    {
        if( m_lastBleStatus != LevoEsp32Ble::CONNECTED )
        {
            // mark value buffer as invalid
            ResetValueBuffer();
        }
        drawBluetoothIcon(m_lastBleStatus);
        ret = true; // indicate changed ble status 
    }

    // trip button status
    updateTripButtonStatus();

    // show battery status every 10 ticks (1000 ms)
    if ((m_showSysStatusCnt++ % 10) == 0)
    {
        M5.Lcd.setTextFont(1);

        // battery image with fill bar
        M5.Lcd.drawBitmap(32, 7, 30, 16, img_battgreen_map);  // 30x16 image
        int rectwidth = (int)(25.0 * m_sysStatus.batteryPercent/100.0);
        uint16_t battFillCol = M5.Lcd.color565(BATTFILL_COLORGREEN);
        if(m_sysStatus.batteryPercent < 20.0 )
            battFillCol = M5.Lcd.color565(BATTFILL_COLORRED);
        else if (m_sysStatus.batteryPercent < 50.0)
            battFillCol = M5.Lcd.color565(BATTFILL_COLORYELL);
        M5.Lcd.fillRect(34, 9, rectwidth, 12, battFillCol );

        // charge symbol
        int xpos = 68;
        M5.Lcd.fillRect(68, 7, 18, 16, BLACK);
        if (m_sysStatus.powerStatus != SystemStatus::POWER_INTERNAL)
        {
            M5.Lcd.drawBitmap(68, 7, 8, 16, img_chargearrow_map);  // 8x16 image
            xpos += 18;
        }

        // battery percent number
        M5.Lcd.setTextColor(M5.Lcd.color565(UNIT_COLOR));
        M5.Lcd.setTextSize(2);
        M5.Lcd.fillRect(xpos, 6, M5.Lcd.width() - xpos -26, 20, BLACK);
        M5.Lcd.setCursor(xpos, 8);
        M5.Lcd.printf("%d%%", m_sysStatus.batteryPercent);

        // sd card symbol
        const uint8_t * imgMap = 0;
        if (m_sysStatus.bHasSDCard )
            imgMap = img_sdgray_map;
        if (m_sysStatus.bLogging )
            imgMap = img_sdgreen_map;
        if( imgMap )
        {   
            int x = xpos + 44;
            M5.Lcd.drawBitmap(x, 6, 15, 18, imgMap);  // 15x18 image
        }

        // calibration state
        if (m_sysStatus.calibrationStatus != SystemStatus::INACTIVE)
        {
            int x = xpos + 44 + 15 + 14;
            if( m_sysStatus.calibrationStatus == SystemStatus::RUNNING )
                M5.Lcd.fillCircle(x, 14, 5, RED);
            else if (m_sysStatus.calibrationStatus == SystemStatus::ABORTED)
                M5.Lcd.fillCircle(x, 14, 5, DARKGREY);
            else if (m_sysStatus.calibrationStatus == SystemStatus::WAITING)
                M5.Lcd.fillCircle(x, 14, 5, WHITE);
            else
                M5.Lcd.fillCircle(x, 14, 5, LIGHTGREY);
        }
        // trip running
        else if (m_sysStatus.tripStatus != SystemStatus::NONE )
        {
            int x = xpos + 44 + 15 + 8;
            if (m_sysStatus.tripStatus == SystemStatus::STARTED )
                M5.Lcd.drawBitmap(x, 6, 15, 18, img_triprun_map);  // 15x18 image
            else
                M5.Lcd.drawBitmap(x, 6, 15, 18, img_tripstop_map);  // 15x18 image
        }

        // warning message or current time
        if( !m_sysStatus.bHasBtPin )
        {
            // "Missing BT pin" hint
            M5.Lcd.setTextColor(TFT_RED);
            M5.Lcd.setCursor(218 - 32, 8);
            M5.Lcd.println("Set pin\xaf");
        }
        else
        {
            // clock display
            char timeStrbuff[20];
            M5.Lcd.setCursor(218 - 32, 8);
            M5.Rtc.GetTime(&m_RTCtime_Now);
            sprintf(timeStrbuff, "%02d:%02d:%02d", m_RTCtime_Now.Hours, m_RTCtime_Now.Minutes, m_RTCtime_Now.Seconds);
            M5.Lcd.println(timeStrbuff);
        }
    }
    return ret;
}

void M5Screen::ShowConfig(Preferences& prefs)
{
    M5ConfigForms forms;
    forms.ShowMenu(prefs);
}
