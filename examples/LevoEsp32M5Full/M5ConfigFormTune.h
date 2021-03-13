/*
 *
 *  Created: 19/02/2021
 *      Author: Bernd Wokoeck
 *
 *  Configuration form for tuning assist levels
 * 
*/

#ifndef M5CONFIG_FORMTUNE_H
#define M5CONFIG_FORMTUNE_H

#include <LevoReadWrite.h>
#include "M5ConfigForms.h"

class M5ConfigFormTune
{
public:
    M5ConfigFormTune( LevoReadWrite& LevoBle);

protected:
    
    // reference to bt communication
    LevoReadWrite& levoBle;
    
    typedef enum
    {
        BUTTON = 0,
        RADIO,
        LABEL,
    } enControlType;

    enum
    {
        // index values of control array, must be in same order
        IDX_TXT1 = 0,   IDX_PEAK_ECO,     IDX_TXT2,       IDX_PEAK_TRAIL,    IDX_TXT3,     IDX_PEAK_TURBO,
        IDX_TXT4,       IDX_ASSIST_ECO,   IDX_TXT5,       IDX_ASSIST_TRAIL,  IDX_TXT6,     IDX_ASSIST_TURBO,
        IDX_TXT7,       IDX_SHUTTLE,      IDX_TXT8,       IDX_ACCELERATION,
        IDX_ASSISTLEV1, IDX_ASSISTLEV2,   IDX_ASSISTLEV3, IDX_BACK,
    };

    typedef struct stScreenControl
    {
        enControlType type;
        Zone z;
        uint8_t textSize;
        const char * text;
        int8_t * pValue;
    } stScreenControl;

    const int bw = 83, lo = bw/2;  // button with and label offset
    const int nElements = sizeof(controls) / sizeof(stScreenControl);
    stScreenControl controls[20] =
    {
        { LABEL,  {  25 + lo,   3,  bw,  16 }, 1, "Peak Eco", NULL }, //labels must preceed button
        { BUTTON, {  25,       16,  bw,  35 }, 1, "", NULL },
        { LABEL,  { 120 + lo,   3,  bw,  16 }, 1, "Peak Trail", NULL },
        { BUTTON, { 120,       16,  bw,  35 }, 1, "", NULL },
        { LABEL,  { 213 + lo,   3,  bw,  16 }, 1, "Peak Turbo", NULL },
        { BUTTON, { 213,       16,  83,  35 }, 1, "", NULL },

        { LABEL,  {  25 + lo,  58,  bw,  16 }, 1, "Assist Eco", NULL },
        { BUTTON, {  25,       71,  bw,  35 }, 1, "", NULL },
        { LABEL,  { 120 + lo,  58,  bw,  16 }, 1, "Assist Trail", NULL },
        { BUTTON, { 120,       71,  bw,  35 }, 1, "", NULL },
        { LABEL,  { 213 + lo,  58,  bw,  16 }, 1, "Assist Turbo", NULL },
        { BUTTON, { 213,       71,  bw,  35 }, 1, "", NULL },

        { LABEL,  {  72 + lo,  124,  bw,  16 }, 1, "Shuttle", NULL },
        { BUTTON, {  72,       139,  bw,  35 }, 1, "", NULL },
        { LABEL,  { 166 + lo,  124,  bw,  16 }, 1, "Acceleration", NULL },
        { BUTTON, { 166,       139,  bw,  35 }, 1, "", NULL },

        { RADIO,  {   5, 195,  57,  35 }, 1, "1", NULL },
        { RADIO,  {  78, 195,  57,  35 }, 1, "2", NULL },
        { RADIO,  { 150, 195,  57,  35 }, 1, "3", NULL },

        { BUTTON, { 234, 195, bw,  35 }, 1, "Back", NULL },
    };

    // buttons in our form
    enum{ nButtons = 12 };
    Button* pButtons[nButtons] = { NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL, NULL,NULL };

    static ButtonColors on_clrs;
    static ButtonColors off_clrs;

    // levo assistance data
    LevoReadWrite::stLevoAssist assistData;
    int8_t assistLevel = (int8_t)LevoReadWrite::enAssistLevel::INVALID;

    bool readAssistData();
    void setButtonLabel( int i, Button & button );
    void setRadioState(int i, Button& button);
    void updateRadioButtons();
    void drawLabels();
    void writeAssistValue(Button& button);

    // polling assist data
    uint32_t tiUpdateAssist = 0L;
    void updateAssistChanged();

    bool onBtPress(Button & button );
    void onAssistLevelChanged(LevoReadWrite::enAssistLevel newLevel );
};

#endif // M5CONFIG_FORMTUNE_H