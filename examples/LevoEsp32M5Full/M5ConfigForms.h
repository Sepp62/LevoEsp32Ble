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

    void ShowMenu(Preferences & prefs);

protected:
    ButtonColors on_clrs = { GREEN, WHITE, WHITE };
    ButtonColors off_clrs = { BLACK, WHITE, WHITE };

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

    void OnCmdBtPin(Preferences& prefs);
    void OnCmdClockWifi(Preferences& prefs);
    void OnCmdScreen(Preferences& prefs);
    void OnCmdClearAll(Preferences& prefs);

    // Message box
    enMsgBoxReturn MsgBox(const char * msg, enMsgBoxButtons type = CONFIRM);
};

#endif // M5CONFIG_FORMS_H