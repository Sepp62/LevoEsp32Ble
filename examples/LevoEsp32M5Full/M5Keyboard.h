// Dervived from GDTouchKeyboard (c) GWENDESIGN.

#ifndef M5KEYBOARD_H
#define M5KEYBOARD_H

#include "arduino.h"

class M5Keyboard
{
public:
    typedef enum
    {
        KEY_MODE_LETTER = 0,
        KEY_MODE_LETTERSHIFT,
        KEY_MODE_NUMBER,
        KEY_MODE_SYM,

        NUM_KEY_MODES,
    } key_mode_t;

  bool Show(String& text, const char* title = NULL, key_mode_t keyMode = KEY_MODE_LETTER);

protected:
    static void _initKeyboard(String text, const char* title, key_mode_t keyMode);
    static void _deinitKeyboard();
    static void _drawKeyboard();
    static void _updateInputText();
    static void _btnAEvent(Event& e);
    static void _buttonEvent(Event& e);
};

#endif // M5KEYBOARD_H
