/*
 *
 *  Created: 01/02/2021
 *      Author: Bernd Woköck
 *
 *  Configuration form for tuning assist levels
 * https://docs.m5stack.com/#/en/arduino/arduino_home_page?id=m5core2_api
 * 
*/

#include <M5Core2.h>
#include <Fonts/EVA_20px.h>
#include "M5ConfigForms.h"
#include "M5Keyboard.h"
#include "M5ConfigFormTune.h"

/* Screen layout
* <Peak Eco>  <Peak Trail>  <Peak Turbo>
*   <Eco>       <Trail>       <Turbo>
*
*      <Shuttle>     <Accel> 
* 
* <Assist1> <Assist2> <Assist3> <Back>
* 
*           <Back>
* */

ButtonColors M5ConfigFormTune::on_clrs = { GREEN, BLACK, WHITE };
ButtonColors M5ConfigFormTune::off_clrs = { BLACK, WHITE, WHITE };

void M5ConfigFormTune::writeAssistValue(Button& button)
{
    uint16_t writeMask = 0;

    int controlIdx = button.userData;
    switch (controlIdx)
    {
    case IDX_PEAK_ECO:
    case IDX_PEAK_TRAIL:
    case IDX_PEAK_TURBO:    writeMask = LevoReadWrite::BIT_PEAKASSIST;    break;
    case IDX_ASSIST_ECO:
    case IDX_ASSIST_TRAIL:
    case IDX_ASSIST_TURBO:  writeMask = LevoReadWrite::BIT_ASSIST;        break;
    case IDX_SHUTTLE:       writeMask = LevoReadWrite::BIT_SHUTTLE;       break;
    case IDX_ACCELERATION:  writeMask = LevoReadWrite::BIT_ACCELSENS;     break;
    }

    if (writeMask != 0)
    {
        levoBle.WriteAssistDataFields(assistData, writeMask);
        ButtonColors off = button.off;
        off.text = TFT_WHITE;
        button.off = off;
    }
}

// check if assist level has been changed on bike
void M5ConfigFormTune::updateAssistChanged()
{
    if (millis() > tiUpdateAssist)
    {
        levoBle.ReadAsync( LevoEsp32Ble::MOT_ASSISTLEVEL, false );
        LevoEsp32Ble::stBleVal bleVal;
        if (levoBle.Update(bleVal))
        {
            if (bleVal.dataType == LevoEsp32Ble::MOT_ASSISTLEVEL)
            {
                int8_t newAssistLevel = (int8_t)bleVal.fVal;
                if( assistLevel != newAssistLevel )
                    onAssistLevelChanged( (LevoReadWrite::enAssistLevel)newAssistLevel );
            }
        }
        tiUpdateAssist = millis() + 500L;
    }
}

void M5ConfigFormTune::updateRadioButtons()
{
    int i;
    for (i = 0; i < nButtons; i++)
    {
        if( pButtons[i]->userData == IDX_ASSISTLEV1 )
            setRadioState(IDX_ASSISTLEV1, *pButtons[i]);
        else if (pButtons[i]->userData == IDX_ASSISTLEV2)
            setRadioState(IDX_ASSISTLEV2, *pButtons[i]);
        else if (pButtons[i]->userData == IDX_ASSISTLEV3)
            setRadioState(IDX_ASSISTLEV3, *pButtons[i]);
    }

}

void M5ConfigFormTune::onAssistLevelChanged(LevoReadWrite::enAssistLevel newLevel )
{
    if( levoBle.WriteAssistLevel( newLevel ) )
    {
        assistLevel = newLevel; // set value for all 3 buttons
        updateRadioButtons();
        M5.Buttons.draw();
    }
}

bool M5ConfigFormTune::onBtPress(Button & button )
{
    // assist level
    if (button.userData == IDX_ASSISTLEV1 && assistLevel != LevoReadWrite::enAssistLevel::ASSIST_ECO)
        onAssistLevelChanged(LevoReadWrite::enAssistLevel::ASSIST_ECO );
    else if (button.userData == IDX_ASSISTLEV2 && assistLevel != LevoReadWrite::enAssistLevel::ASSIST_TRAIL)
        onAssistLevelChanged(LevoReadWrite::enAssistLevel::ASSIST_TRAIL );
    else if (button.userData == IDX_ASSISTLEV3 && assistLevel != LevoReadWrite::enAssistLevel::ASSIST_TURBO)
        onAssistLevelChanged(LevoReadWrite::enAssistLevel::ASSIST_TURBO );

    // assistance data
    int controlIdx = button.userData;
    if( controls[controlIdx].type == BUTTON && controlIdx > 0 )
    {
        String text;
        M5Keyboard keyb;
        if ( keyb.Show( text, controls[controlIdx - 1].text, M5Keyboard::KEY_MODE_NUMERIC ) )
        {
            if (controls[controlIdx].pValue)
            {
                // validate range
                int8_t value = atoi( text.c_str() );
                if( value < 0 )
                    value = 0;
                else if( value > 100 )
                    value = 0;
                // set value in write buffer
                *controls[controlIdx].pValue = value;
                // adapt button display
                setButtonLabel( controlIdx, button );
                // mark color as changed and redraw
                ButtonColors off = button.off;
                off.text = TFT_GREEN;
                button.off = off;
                drawLabels();
                // write changes back
                M5.Buttons.draw();
                writeAssistValue( button );
            }
        }
        drawLabels();
        M5.Buttons.draw();
    }
}

bool M5ConfigFormTune::readAssistData()
{
    if (levoBle.ReadAssistDataFields( assistData) )
    {
        controls[IDX_PEAK_ECO].pValue   = &assistData.peakAssist[0];
        controls[IDX_PEAK_TRAIL].pValue = &assistData.peakAssist[1];
        controls[IDX_PEAK_TURBO].pValue = &assistData.peakAssist[2];

        controls[IDX_ASSIST_ECO].pValue   = &assistData.assist[0];
        controls[IDX_ASSIST_TRAIL].pValue = &assistData.assist[1];
        controls[IDX_ASSIST_TURBO].pValue = &assistData.assist[2];

        controls[IDX_SHUTTLE].pValue      = &assistData.shuttle;
        controls[IDX_ACCELERATION].pValue = &assistData.accelSens;

        assistLevel = levoBle.GetAssistLevel();
        controls[IDX_ASSISTLEV1].pValue = &assistLevel; // all 3 buttons share the same value
        controls[IDX_ASSISTLEV2].pValue = &assistLevel;
        controls[IDX_ASSISTLEV3].pValue = &assistLevel;

        return true;
    }

  return false;
}

void M5ConfigFormTune::setButtonLabel(int i, Button & button )
{
    if (controls[i].pValue) // show value attached to the control
    {
        int8_t value = *controls[i].pValue;
        if( value < 0 )
            return;

        char label[50];
        snprintf(label, 50, "%d %%", value);
        button.setLabel(label);
    }
    else
        button.setLabel(controls[i].text); // normal text button
}

void M5ConfigFormTune::setRadioState(int controlIdx, Button& button)
{
    if (controls[controlIdx].pValue)
    {
        int8_t value = *controls[controlIdx].pValue;
        if (value < 0)
            return;

        switch (controlIdx)
        {
        case IDX_ASSISTLEV1: 
            button.off = (value == 1) ? on_clrs  : off_clrs;
            button.on  = (value == 1) ? off_clrs : on_clrs;
            break;
        case IDX_ASSISTLEV2:
            button.off = (value == 2) ? on_clrs : off_clrs;
            button.on = (value == 2) ? off_clrs : on_clrs;
            break;
        case IDX_ASSISTLEV3:
            button.off = (value == 3) ? on_clrs : off_clrs;
            button.on = (value == 3) ? off_clrs : on_clrs;
            break;
        }
    }
    button.setLabel(controls[controlIdx].text);
}

void M5ConfigFormTune::drawLabels()
{
    M5.Lcd.setFreeFont(NULL); // set to built in GLCD font
    M5.Lcd.setTextDatum(TC_DATUM);

    int i;
    for (i < 0; i < nElements; i++)
    {
        M5.Lcd.setTextSize(controls[i].textSize);
        // draw label
        if (controls[i].type == LABEL)
        {
            M5.Lcd.setTextColor(TFT_GREEN);
            M5.Lcd.drawString(controls[i].text, controls[i].z.x, controls[i].z.y);
        }
    }
}

M5ConfigFormTune::M5ConfigFormTune(LevoReadWrite& LevoBle) : levoBle( LevoBle )
{
    // wait for TAP end
    while (M5.Touch.ispressed())
        M5.update();

    bool bDone = false;

    M5.Lcd.pushState();
    M5.Lcd.clear(TFT_BLACK);
    M5.Lcd.setFreeFont(NULL); // set to built in GLCD font
    M5.Lcd.setTextDatum(TC_DATUM);

    if (!levoBle.IsConnected())
    {
        M5ConfigForms::MsgBox("Connect your bike!");
        return;
    }

    // prepare for reading
    bool bResubscribe = levoBle.IsSubscribed();
    levoBle.Unsubscribe();

    //show message
    int x = M5.Lcd.width() / 2;
    int y = M5.Lcd.height() / 2 - 10;
    M5.Lcd.setTextSize( 2 );
    M5.Lcd.drawString("Reading data...", x, y );

    // read assist data from levo
    if (!readAssistData())
    {
        M5ConfigForms::MsgBox("Error reading data!");
        return;
    }

    M5.Lcd.clear(TFT_BLACK);

    // create buttons
    int i, btIdx;
    for (i < 0, btIdx = 0; i < nElements; i++)
    {
        // draw button
        M5.Lcd.setTextSize(controls[i].textSize);
        if(controls[i].type == BUTTON )
        {
            if( btIdx < nButtons )
            {
                pButtons[btIdx] = new Button( controls[i].z.x, controls[i].z.y, controls[i].z.w, controls[i].z.h, false, "", off_clrs, on_clrs, TC_DATUM, 0, 3, 0xFF );
                pButtons[btIdx]->userData = i; // remember control array idx 
                setButtonLabel(i, *pButtons[btIdx]);
                btIdx++;
            }
        }
        else if ( controls[i].type == RADIO)
        {
            if (btIdx < nButtons)
            {
                pButtons[btIdx] = new Button(controls[i].z.x, controls[i].z.y, controls[i].z.w, controls[i].z.h, false, "", off_clrs, on_clrs, TC_DATUM, 0, 3, 0);
                pButtons[btIdx]->userData = i; // remember control array idx 
                setRadioState(i, *pButtons[btIdx]);
                btIdx++;
            }
        }
    }
    drawLabels();
    M5.Buttons.draw();

    // reset poll time for assist data
    tiUpdateAssist = 0L;

    // check buttons pressed
    while (!bDone)
    {
        M5.update();

        if (M5.Touch.ispressed())
        {
            for (i = 0; i < nButtons; i++)
            {
                if (pButtons[i] && pButtons[i]->wasPressed())
                {
                    if(pButtons[i]->userData == IDX_BACK )  // back button
                    {
                        bDone = true;
                        break;
                    }
                    else
                        onBtPress( *pButtons[i] );  // all other buttons
                }
            }
        }
        else
        {
            // updateAssistChanged(); // needs too much time and makes GUI laggy
        }

        if( !levoBle.IsConnected() ) // exit w/o BT connection
            break;
    }

    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

    // cleanup
    for (i = 0; i < nButtons; i++)
    {
        if( pButtons[i] )
        {
            pButtons[i]->erase();
            delete pButtons[i];
            pButtons[i] = NULL;
        }
    }

    M5.Buttons.draw();
    M5.Lcd.popState();

    M5.Lcd.clear(TFT_BLACK);

    if( !bDone )
        M5ConfigForms::MsgBox("Connection lost!");

    if(bResubscribe )
        levoBle.Subscribe();
}
