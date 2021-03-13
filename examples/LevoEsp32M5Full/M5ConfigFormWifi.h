/*
 *
 *  Created: 19/02/2021
 *      Author: Bernd Wokoeck
 *
 *  WIFI selection form
 *
 */

#ifndef M5CONFIG_FORMWIFI_H
#define M5CONFIG_FORMWIFI_H

#include <M5Core2.h>

class M5ConfigFormWifi
{
public:
    bool SelectAccessPointPage(String& ssid);

protected:
    ButtonColors on_clrs = { GREEN, WHITE, BLACK };
    ButtonColors off_clrs = { BLACK, WHITE, BLACK };

    ButtonColors on_clrs2 = { GREEN, M5.Lcd.color565(130, 130, 130), WHITE };
    ButtonColors off_clrs2 = { BLACK, M5.Lcd.color565(130, 130, 130), WHITE };
};

#endif // M5CONFIG_FORMWIFI_H