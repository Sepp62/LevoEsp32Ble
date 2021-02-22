/*
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 *  Configuration forms for settings
 * 
 * https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api
*/

#include <M5Core2.h>
#include <Fonts/EVA_20px.h>
#include "M5ConfigForms.h"
#include "M5NTPTime.h"
#include "M5Keyboard.h"
#include "M5ConfigFormWifi.h"
#include "FileLogger.h"
#include "AltimeterBMP280.h"

ButtonColors M5ConfigForms::on_clrs = { GREEN, WHITE, WHITE };
ButtonColors M5ConfigForms::off_clrs = { BLACK, WHITE, WHITE };

// simple full screen centered two line message
M5ConfigForms::enMsgBoxReturn M5ConfigForms::MsgBox(const char* msg, enMsgBoxButtons type)
{
    enMsgBoxReturn ret = RET_CANCEL;

    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    // print message
    M5.Lcd.clear(TFT_BLACK);
    M5.Lcd.pushState();

    TFT_eSprite lcdbuff = TFT_eSprite(&M5.Lcd);
    lcdbuff.createSprite(320, 240);
    lcdbuff.setFreeFont(&EVA_20px);
    lcdbuff.setTextSize(1);
    lcdbuff.setTextColor(TFT_WHITE, TFT_BLACK);
    lcdbuff.setTextDatum(TC_DATUM);
    int x = M5.Lcd.width() / 2;
    int y = M5.Lcd.height() / 2 - 10;
    lcdbuff.drawString( msg, x, y, GFXFF);
    lcdbuff.pushSprite(0, 0);

    // show buttons
    Button * b1 = NULL, * b2 = NULL;
    const int cx = 100, cy = 35;
    const int bx = M5.Lcd.width()/4 - cx/2;
    const int by = M5.Lcd.height()/4 * 3;
    switch (type)
    {
    case CONFIRM:
        b1 = new Button(x - cx / 2, by, cx, cy, false, "OK", off_clrs, on_clrs, TC_DATUM, 0, 3);
        break;
    case OKCANCEL:
        b1 = new Button(bx, by, cx, cy, false, "OK", off_clrs, on_clrs, TC_DATUM, 0, 3);
        b2 = new Button((bx + cx / 2) * 3 - cx / 2, by, cx, cy, false, "Cancel", off_clrs, on_clrs, TC_DATUM, 0, 3);
        break;
    case YESNO:
        b1 = new Button(bx, by, cx, cy, false, "Yes", off_clrs, on_clrs, TC_DATUM, 0, 3);
        b2 = new Button((bx + cx / 2) * 3 - cx / 2, by, cx, cy, false, "No", off_clrs, on_clrs, TC_DATUM, 0, 3);
        break;
    }
    M5.Buttons.draw();

    // check pressed
    while (1)
    {
        M5.update();
        if ( b1 && b1->wasPressed() )
        {
            if( type == CONFIRM ||type == OKCANCEL)
                ret = RET_OK;
            else
                ret = RET_YES;
            break;
        }
        else if (b2 && b2->wasPressed())
        {
            if ( type == OKCANCEL)
                ret = RET_CANCEL;
            else
                ret = RET_NO;
            break;
        }
    }

    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    // cleanup
    if( b1 )
    {
        b1->erase();
        delete b1;
    }
    if( b2 )
    {
        b2->erase();
        delete b2;
    }

    M5.Lcd.popState();

    return ret;
}

// set button colors due to check state
void M5ConfigForms::CheckButtons(int nPressed, stItem* pItems, int nItems, Button* pButtons[])
{
    int i;
    for (i = 0; i < nItems; i++)
    {
        if (pItems[i].type == CHECKBOX || pItems[i].type == RADIO)
        {
            if (nPressed >= 0) // not during initialization
            {
                if (i == nPressed)
                    pItems[i].bChecked = pItems[i].bChecked ? false : true; // toggle state
                else if (pItems[i].type == RADIO)
                    pItems[i].bChecked = false;   // uncheck other radio buttons
            }

            if (pButtons[i])
            {
                if (pItems[i].bChecked)
                    pButtons[i]->off = { GREEN, BLACK, DARKGREY }; // bg text outline
                else
                    pButtons[i]->off = { BLACK, WHITE, DARKGREY }; // bg text outline
            }
        }
    }
}

// show check item page (max. 5 items)
int M5ConfigForms::PageCheckMenu(stItem* pItems, int nItems)
{
    int cmd = 0;
    bool bDone = false;

    // wait for TAP end
    while (M5.Touch.ispressed())
        M5.update();

    M5.Lcd.pushState();
    M5.Lcd.clear(TFT_BLACK);

    // buttons/menu items
    int i;
    const int cx = 250, cy = 35;
    int yinc = 200 / nItems;  // 50 pixel @4 buttons
    int x = M5.lcd.width() / 2 - cx / 2, y = 32;
    const int maxButtons = 5;

    nItems = min(nItems, maxButtons);
    Button* pButtons[maxButtons] = { NULL,NULL,NULL,NULL,NULL };
    for (i = 0; i < nItems; i++)
    {
        if (pItems[i].type == BUTTON)
        {
            pButtons[i] = new Button(x, y + (i * yinc), cx, cy, false, "", off_clrs, on_clrs, TC_DATUM, 0, 3);
            pButtons[i]->setLabel(pItems[i].label);
        }
        else if (pItems[i].type == CHECKBOX || pItems[i].type == RADIO)
        {
            pButtons[i] = new Button(x, y + (i * yinc), cx, cy, false, "", off_clrs, on_clrs, TC_DATUM, 0, 3, 0);
            pButtons[i]->setLabel(pItems[i].label);
            CheckButtons(-1, pItems, nItems, pButtons);
        }
        else if (pItems[i].type == NUMINPUT || pItems[i].type == TXTINPUT)
        {
            pButtons[i] = new Button(x, y + (i * yinc), cx, cy, false, "", off_clrs, on_clrs, TC_DATUM, 0, 3);
            char label[51];
            snprintf(label, sizeof(label), "%s: %s", pItems[i].label, pItems[i].value);
            pButtons[i]->setLabel(label);
        }
    }
    M5.Buttons.draw();

    // check pressed
    while (!bDone)
    {
        M5.update();
        for (i = 0; i < nItems; i++)
        {
            if (pButtons[i]->wasPressed())
            {
                if (pItems[i].type == BUTTON)
                {
                    cmd = pItems[i].cmdId;
                    bDone = true;
                    break;
                }
                else if (pItems[i].type == CHECKBOX || pItems[i].type == RADIO)
                {
                    CheckButtons(i, pItems, nItems, pButtons);
                    M5.Buttons.draw();
                    break;
                }
                else if (pItems[i].type == NUMINPUT || pItems[i].type == TXTINPUT)
                {
                    String text;
                    M5Keyboard::key_mode_t keyMode = (pItems[i].type == NUMINPUT) ? M5Keyboard::KEY_MODE_NUMBER : M5Keyboard::KEY_MODE_LETTER;
                    M5Keyboard keyb;
                    if (keyb.Show(text, pItems[i].label, keyMode))
                    {
                        char label[51];
                        snprintf(label, sizeof(label), "%s: %s", pItems[i].label, text.c_str());
                        snprintf(pItems[i].value, sizeof(pItems[i].lenValue), "%s", text.c_str());
                        pButtons[i]->setLabel(label);
                    }
                    M5.Buttons.draw();
                }
            }
        }
    }

    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    // cleanup
    for (i = 0; i < nItems; i++)
    {
        pButtons[i]->erase();
        delete pButtons[i];
    }

    M5.Lcd.popState();
    M5.Lcd.clear(TFT_BLACK);

    return cmd;
}

// enter bluetooth pin
void M5ConfigForms::OnCmdBtPin(Preferences& prefs)
{
    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    Serial.println("btPin...");
    String str = "";
    M5Keyboard keyb;
    bool ret = keyb.Show( str, "Enter bluetooth pin:", M5Keyboard::KEY_MODE_NUMERIC );
    Serial.println( str );
    if ( ret )
    {
        uint32_t pin = atol(str.c_str());
        if( pin )
        {
            Serial.println("...written to prefs");
            prefs.putULong("BtPin", atol(str.c_str()));
        }
        else
        {
            Serial.println("...deleted from prefs");
            prefs.remove("BtPin");
        }
        // show restart message
        MsgBox("Bluetooth pin changed. Restart!");

        prefs.end();
        ESP.restart(); // brute force
    }
}

// clear preferences
void M5ConfigForms::OnCmdClearAll(Preferences& prefs)
{
    if( MsgBox("*REALLY* delete all settings?", YESNO ) == RET_YES )
    {
        prefs.clear();
        prefs.end();
        ESP.restart(); // brute force
    }
}

// set RTC from NTP server (Wifi internet connection)
void M5ConfigForms::OnCmdClockWifi(Preferences& prefs)
{
    String ssid;
    String pwd;
    bool bPwdChanged = false;

    // read preferences for ssid and pwd
    ssid = prefs.getString("SSID");
    if( !ssid.isEmpty())
        pwd = prefs.getString( "WifiPwd" );

RETRY:
    // if there is no ssid and pwd, select it
    if( pwd.isEmpty() )
    {
      M5ConfigFormWifi configWifi;
      if (configWifi.SelectAccessPointPage(ssid))
      {
          // Serial.println( ssid );
          if (!ssid.isEmpty())
          {
              pwd = prefs.getString("WifiPwd");
              if (!pwd.isEmpty())
              {
                  if (MsgBox("Use stored password?", YESNO) == RET_NO)
                      pwd = "";
              }
              if (pwd.isEmpty())
              {
                  String text;
                  M5Keyboard keyb;
                  if (keyb.Show(text, "Enter Wifi password:"))
                  {
                      pwd = text;
                      bPwdChanged = true;
                  }
              }
          }
      }
    }

    // credentials missing
    if (ssid.isEmpty() || pwd.isEmpty())
    {
        if (MsgBox("No credentials. Retry?", YESNO) == RET_YES)
            goto RETRY;
        else
            return;
    }

    // now we can test
    Serial.println("Testing wifi");
    MsgBox( "Start testing Wifi connection");
    WiFi.begin(ssid.c_str(), pwd.c_str());
    uint32_t to = millis() + 10000L;
    while (WiFi.status() != WL_CONNECTED)
    {
        if( millis() > to )
        {
            if( MsgBox("Failed! Select another one?", YESNO ) == RET_YES )
            {
                pwd = "";
                goto RETRY;
            }
            else
              return;
        }
        delay( 100 );
    }

    // connected, write prefs
    prefs.putString("SSID", ssid);
    if( bPwdChanged && MsgBox("Store Wifi Pwd?", YESNO ) == RET_YES )
        prefs.putString("WifiPwd", pwd );

    Serial.println("Set NTP time");

    // finally set time
    M5NTPTime NtpTime;
    if( NtpTime.SetTime(ssid.c_str(), pwd.c_str() ) )
        MsgBox( "Internet time set!" );
    else
        MsgBox("Error setting time!");
}

void M5ConfigForms::OnCmdScreen(Preferences& prefs)
{
    int cmd = 0;

    stItem items[3] =
    {
        { 1, NUMINPUT, false, "Backlight timeout", NULL, 0 },
        { 2, CHECKBOX, false, "Backlight on while charging", NULL, 0 },
        { 0, BUTTON,   false, "Back", NULL, 0 },
    };

    // backlight timeout
    char blTo[10];
    uint16_t to = prefs.getUShort( "BacklightTo", 60 );
    snprintf(blTo, sizeof( blTo), "%d", to );
    items[0].value = blTo;
    items[0].lenValue = sizeof( blTo );

    // backlight on charging
    bool blChg = items[1].bChecked = prefs.getUChar("BacklightChg", 1) ? true : false;

    while ((cmd = PageCheckMenu(items, N_ELE(items))) != 0)
    {
        ;
    }

    // backlight timeout
    uint16_t to2 = atoi( items[0].value );
    if( to != to2 )
    { 
        if( to2 > 3600 ) to2 = 3600;
        if( to2 < 10 ) to2 = 10;
        prefs.putUShort("BacklightTo", to2 );
    }

    // backlight on charging
    if( items[1].bChecked != blChg )
        prefs.putUChar("BacklightChg", items[1].bChecked ? 1 : 0 );
}

void M5ConfigForms::OnCmdLogging(Preferences& prefs)
{
    int i;
    stItem items[5] =
    {
        { FileLogger::SIMPLE,     RADIO,  false, "Text (complete)", NULL, 0 },
        { FileLogger::CSV_SIMPLE, RADIO,  false, "CSV (complete)", NULL, 0 },
        { FileLogger::CSV_KNOWN,  RADIO,  false, "CSV (known data only)", NULL, 0 },
        { FileLogger::CSV_TABLE,  RADIO,  false, "CSV all values per line", NULL, 0 },
        { 0,                      BUTTON, false, "Back", NULL, 0 },
    };

    // get stored format
    uint8_t logFormat = prefs.getUChar( "LogFormat" );
    if( logFormat >= FileLogger::NUM_FORMATS || logFormat == FileLogger::NONE )
        logFormat = FileLogger::SIMPLE;

    Serial.printf("Log format from preferences: %d\r\n", logFormat);

    // check item (cmdId coresponds to log format constant)
    for (i = 0; i < N_ELE(items); i++)
    {
        if (items[i].cmdId == logFormat )
        {
            items[ i ].bChecked = true;
            break;
        }
    }

    // show radio buttons
    PageCheckMenu(items, N_ELE(items));

    // write checked item to preferences
    for( i = 0; i < N_ELE(items); i++ )
    {
        if( items[i].bChecked && logFormat != items[i].cmdId )
        {
            prefs.putUChar("LogFormat", items[i].cmdId );
            break;
        }
    }
}

// altimeter calibration
void M5ConfigForms::OnCmdAltimeter(Preferences& prefs)
{
    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    if (!AltimeterBMP280::CanCalculateSealevel())
    {
        MsgBox( "No altimeter hardware found!" );
        return;
    }

    String str = "";
    M5Keyboard keyb;
    bool ret = keyb.Show(str, "Enter current altitude (m):", M5Keyboard::KEY_MODE_NUMERIC);
    Serial.println(str);
    if (ret)
    {
        uint32_t altitude = atol(str.c_str());
        if (altitude)
        {
            float sealevelhPa = AltimeterBMP280::GetCurrentSealevelhPa((float)altitude);
            if(sealevelhPa != 0.0 )
            {
                prefs.putFloat("sealevelhPa", sealevelhPa);
                Serial.printf("SealevelPressure (hPa): %f\r\n", sealevelhPa);
            }
        }
    }
}

// command handler for 2nd more menu buttons
void M5ConfigForms::OnCmdMore2(Preferences& prefs)
{
    int cmd = 0;

    stItem items[3] =
    {
        { 1, BUTTON, false, "Set clock/Wifi", NULL, 0 },
        { 2, BUTTON, false, "Clear settings", NULL, 0 },
        { 0, BUTTON, false, "Back", NULL, 0 },
    };

    while ((cmd = PageCheckMenu(items, N_ELE(items))) != 0)
    {
        // Serial.println(cmd);
        switch (cmd)
        {
        case 1: OnCmdClockWifi(prefs); break;
        case 2: OnCmdClearAll(prefs);  break;
        }
    }
}

// command handler for more menu buttons
void M5ConfigForms::OnCmdMore(Preferences& prefs)
{
    int cmd = 0;

    stItem items[5] =
    {
        { 1, BUTTON, false, "Bluetooth Pin", NULL, 0 },
        { 2, BUTTON, false, "Altimeter", NULL, 0 },
        { 3, BUTTON, false, "Screen", NULL, 0 },
        { 4, BUTTON, false, "More...", NULL, 0 },
        { 0, BUTTON, false, "Back", NULL, 0 },
    };

    while ((cmd = PageCheckMenu( items, N_ELE( items ))) != 0)
    {
        // Serial.println(cmd);
        switch (cmd)
        {
        case 1: OnCmdBtPin(prefs);     break;
        case 2: OnCmdAltimeter(prefs); break;
        case 3: OnCmdScreen(prefs);    break;
        case 4: OnCmdMore2(prefs);     break;
        }
    }
}

// entry point and command handler for menu buttons
void M5ConfigForms::ShowMenu(Preferences& prefs)
{
    int cmd = 0;

    stItem items[4] =
    {
        { 1, CHECKBOX, false, "Bluetooth on", NULL, 0 },
        { 2, BUTTON,   false, "Log file format", NULL, 0 },
        { 3, BUTTON,   false, "More...", NULL, 0 },
        { 0, BUTTON,   false, "Back", NULL, 0 },
    };

    // bluetooth enabled
    bool bBtEnabled = items[0].bChecked = prefs.getUChar( "BtEnabled", 1 ) ? true : false;

    while ((cmd = PageCheckMenu(items, N_ELE(items))) != 0)
    {
        // Serial.println(cmd);
        switch (cmd)
        {
        case 2: OnCmdLogging(prefs);     break;
        case 3: OnCmdMore( prefs );      break;
        }
    }

    // write  bluetooth enabled state
    if( items[0].bChecked != bBtEnabled );
        prefs.putUChar("BtEnabled", items[0].bChecked ? 1 : 0 );
}
