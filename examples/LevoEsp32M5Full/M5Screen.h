#ifndef M5_SCREEN_H
#define M5_SCREEN_H

#include <M5Core2.h>
#include <Preferences.h>
#include "DisplayData.h"
#include "M5Field.h"
#include "SystemStatus.h"

class M5Screen
{
protected:
    const int START_Y = 32;  // Y start position on screen

    typedef struct
    {
        int x;
        int y;
        M5Field* pField;
    } stRender;

    stRender fldRender[DisplayData::numElements];

    int idToIdx[DisplayData::numElements] = {0,0,0,0,0,0,0,0,0,0,0,0,0};

    void RenderEmptyValue(stRender& render, const DisplayData::stDisplayData* pDesc );

    // counter for showing system Status to slow down refresh rate
    uint32_t  showSysStatusCnt = 0L;

    RTC_TimeTypeDef RTCtime_Now;

public:
    void Init(DisplayData& dispData);
    void ShowValue( DisplayData::enIds id, float val, DisplayData& dispData );
    bool ShowSysStatus(SystemStatus& sysStatus);
    void ShowConfig(Preferences& prefs);
};

#endif // M5_SCREEN_H