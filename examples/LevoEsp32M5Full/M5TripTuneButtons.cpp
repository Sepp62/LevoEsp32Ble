/*
 *
 *  Created: 27/02/2021
 *      Author: Bernd Wokoeck
 *
 * Button bar with Trip and Tune buttons
 *
 */

#include <M5Core2.h>
#include "M5TripTuneButtons.h"

void M5TripTuneButtons::SetButtonState(enButtons btId, bool bChecked)
{
    int buttonBit = 1<<btId;
    bool bIsSet = (stateMask & buttonBit) ? true : false;
    if( bIsSet == bChecked )
        return; 

    if (btId < nButtons && pButtons[btId] != NULL )
    {
        pButtons[btId]->on  = bChecked ? off_clrs : on_clrs;
        pButtons[btId]->off = bChecked ? on_clrs  : off_clrs;
        pButtons[btId]->draw();
        if( bChecked )
            stateMask |= buttonBit;
        else
            stateMask &= ~buttonBit;
    }
}

// enable/disable button bar on bottom of screen
void M5TripTuneButtons::Enable(void (*fnBtEvent)(Event&))
{
    int i;
    if ( fnBtEvent != NULL )
    {
        createButtons();
        for (i = 0; i < nButtons; i++)
        {
            pButtons[i]->on  = on_clrs;
            pButtons[i]->off = off_clrs;
            pButtons[i]->addHandler(fnBtEvent, E_TOUCH);
            pButtons[i]->draw();
        }
    }
    else
        deleteButtons();
}

void M5TripTuneButtons::createButtons()
{
    const int START_Y = 188;
    deleteButtons();
    pButtons[BTSTART] = new Button(   6, START_Y, 68, 28, false, "Start",  { NODRAW, NODRAW, NODRAW }, { NODRAW, NODRAW, NODRAW }, TC_DATUM, 0, 0, 0xFF );
    pButtons[BTSTOP]  = new Button(  90, START_Y, 68, 28, false, "Stop",   { NODRAW, NODRAW, NODRAW }, { NODRAW, NODRAW, NODRAW }, TC_DATUM, 0, 0, 0xFF);
    pButtons[BTRESET] = new Button( 170, START_Y, 68, 28, false, "Finish", { NODRAW, NODRAW, NODRAW }, { NODRAW, NODRAW, NODRAW }, TC_DATUM, 0, 0, 0xFF);
    pButtons[BTTUNE]  = new Button( 248, START_Y, 68, 28, false, "Tune",   { NODRAW, NODRAW, NODRAW }, { NODRAW, NODRAW, NODRAW }, TC_DATUM, 0, 0, 0xFF);
    pButtons[BTSTART]->userData = BTSTART;
    pButtons[BTSTOP]->userData  = BTSTOP;
    pButtons[BTRESET]->userData = BTRESET;
    pButtons[BTTUNE]->userData  = BTTUNE;
    stateMask = 0;
};

void M5TripTuneButtons::deleteButtons()
{
    int i;
    for (i = 0; i < nButtons; i++)
    {
        if( pButtons[i] != NULL )
        {
            pButtons[i]->delHandlers();
            pButtons[i]->erase();
            delete pButtons[i];
            pButtons[i] = NULL;
        }
    }
}

