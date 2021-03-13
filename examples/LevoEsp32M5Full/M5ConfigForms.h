/*
 *  Created: 19/02/2021
 *      Author: Bernd Wokoeck
 *
 *  Configuration forms for settings
 *
*/

#ifndef M5CONFIG_FORMS_H
#define M5CONFIG_FORMS_H

#include <M5Core2.h>
#include <Preferences.h>

class M5ConfigForms
{
public:
    typedef enum
    {
        CONFIRM = 0,
        OKCANCEL,
        YESNO,
    } enMsgBoxButtons;

    typedef enum
    {
        RET_OK = 0,
        RET_CANCEL,
        RET_YES,
        RET_NO,
    } enMsgBoxReturn;

    // main entry point for config menues
    void ShowMenu(Preferences & prefs);

    // Message box
    static enMsgBoxReturn MsgBox(const char* msg, enMsgBoxButtons type = CONFIRM);

protected:
    static ButtonColors on_clrs;
    static ButtonColors off_clrs;

    // menu items
    typedef enum
    {
        BUTTON = 0,
        CHECKBOX,
        RADIO,
        TXTINPUT,
        NUMINPUT,
    } enItemType;

    typedef struct stitem
    {
        int          cmdId;
        enItemType   type;
        bool         bChecked;
        const char * label;
        char *       value;
        size_t       lenValue;
    } stItem;
    #define N_ELE(a) (sizeof(a)/sizeof( stItem ))

    int PageCheckMenu(stItem* pItems, int nItems);
    void CheckButtons( int nPressed, stItem* pItems, int nItems, Button * pButtons[] );

    // subPages
    void OnCmdLogging(Preferences& prefs);
    void OnCmdMore(Preferences& prefs);
    void OnCmdMore2(Preferences& prefs);

    void OnCmdBtPin(Preferences& prefs);
    void OnCmdClockWifi(Preferences& prefs);
    void OnCmdScreen(Preferences& prefs);
    void OnCmdClearAll(Preferences& prefs);
    void OnCmdAltimeter(Preferences& prefs);
};

#endif // M5CONFIG_FORMS_H