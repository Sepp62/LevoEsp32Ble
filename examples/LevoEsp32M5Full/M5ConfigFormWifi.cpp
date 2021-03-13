/*
 * 
 *  Created: 01/02/2021
 *      Author: Bernd Wokoeck
 *
 *  WIFI selection form
 * 
 */

#include <WiFi.h>
#include "M5ConfigFormWifi.h"

bool M5ConfigFormWifi::SelectAccessPointPage( String & ssid )
{
    int i = 0;
    const int NBUTTONS = 5;
    const int LIST_Y = 25;
    bool bDone = false;
    int nScanCycles = 0;

    M5.Lcd.pushState();
    M5.Lcd.clear(TFT_BLACK);

    bool hasFirstResult = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.scanNetworks(true);

    // header
    M5.Lcd.setFreeFont( FF1 );
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor( M5.Lcd.color565(130, 130, 130) );
    M5.Lcd.drawString( "Select Wifi network:", 2, 2 );
    M5.Lcd.setTextSize(2);

    String ssids[NBUTTONS];
    Button* pButtons[NBUTTONS] = { NULL, NULL, NULL, NULL, NULL };
    const int cx = M5.Lcd.width() - 10, cy = M5.Lcd.fontHeight();

    // 5 button list
    for( i = 0; i < NBUTTONS; i++ )
        pButtons[i] = new Button(2, i * cy + LIST_Y, cx, cy, false, "", off_clrs, on_clrs, TL_DATUM, 0, 3, 0);
    
    // back button
    Button btBack( M5.Lcd.width()/2 - 50, M5.Lcd.height() - 35, 100, cy - 5, false, "Back", off_clrs2, on_clrs2, TC_DATUM, 0, 3 );

    // do scan
    delay(100);
    while (1)
    {
        int scanCount = WiFi.scanComplete();
        if (scanCount == WIFI_SCAN_FAILED )
        {
            Serial.println("Wifi scan failed");
        }
        else if (scanCount == WIFI_SCAN_RUNNING)
        {
            // Serial.println("Wifi scanning");
            if (!hasFirstResult)
            {
                nScanCycles++;
                // show scanning... animation since we have no hits 
                M5.Lcd.setFreeFont(FF1);
                M5.Lcd.setTextSize(1);
                M5.Lcd.setTextColor(M5.Lcd.color565(130, 130, 130));
                M5.Lcd.setCursor( 100, 110 );
                M5.Lcd.print("Scanning");
                M5.Lcd.fillRect(M5.Lcd.getCursorX(), 110, 50, 30, TFT_BLACK);
                for( int z = 0; z < ((nScanCycles/20) % 4); z++)
                    M5.Lcd.print(".");
            }
        }
        else if (scanCount == 0)
        {
            Serial.println("no networks found");
            // M5.Lcd.drawString("No networks found", 15, 22);
        }
        else
        {
            // draw network names (ssid)
            for (int i = 0; i < min( scanCount, NBUTTONS); i++)
            {
                String SSIDStr = WiFi.SSID(i);
                ssids[i] = SSIDStr;
                // limit display name length
                if (SSIDStr.length() > 45)
                {
                    SSIDStr = SSIDStr.substring(0, 42);
                    SSIDStr += "...";
                }

                int32_t rssi = (WiFi.RSSI(i) < -100) ? -100 : WiFi.RSSI(i);
                rssi = (rssi + 100) / 10;

                // append rssi bar
                String ssidBar = " ";
                for( int j = 0; j < rssi; j++ )
                    ssidBar += '=';
                SSIDStr += ssidBar;

                // set network name as button label
                pButtons[i]->setLabel(SSIDStr.c_str());
            }
            M5.Buttons.draw();

            WiFi.scanDelete();
            WiFi.scanNetworks(true);
            hasFirstResult = true;
        }

        // check pressed
        M5.update();
        for (i = 0; i < NBUTTONS; i++)
        {
            if( pButtons[i]->wasPressed() )
            {
                ssid = ssids[i];
                bDone = true;
                break;
            }
        }

        if (btBack.wasPressed())
            break;

        if( bDone )
            break;

        delay(10);
    }

    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    // cleanup
    for ( i = 0; i < NBUTTONS; i++)
    {
        pButtons[i]->erase();
        delete pButtons[i];
    }

    M5.Lcd.popState();
    return !ssid.isEmpty();
}
