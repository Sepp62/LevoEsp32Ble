/*
 *
 *  Created: 27/02/2021
 *      Author: Bernd Wokoeck
 *
 * Button bar with Trip and Tune buttons
 *
 */

#ifndef M5TRIPTUNE_BUTTONS_H
#define M5TRIPTUNE_BUTTONS_H

class M5TripTuneButtons
{
public:
    typedef enum
    {
        BTSTART = 0,
        BTSTOP,
        BTRESET,
        BTTUNE,
    } enButtons;

    void Enable( void (*fnBtEvent)(Event&) );
    void SetButtonState(enButtons btId, bool bChecked);

protected:
    // bottom button bar: Trip Start/Stop/Reset, Tune
    ButtonColors on_clrs = { GREEN, TFT_BLACK, TFT_DARKGREY };
    ButtonColors off_clrs = { BLACK, TFT_DARKGREY, TFT_DARKGREY };

    int stateMask = 0;

    enum { nButtons = 4 };
    Button* pButtons[nButtons] = { NULL,NULL,NULL,NULL};

    void createButtons();
    void deleteButtons();
};

#endif // M5TRIPTUNE_BUTTONS_H