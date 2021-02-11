// Dervived from GDTouchKeyboard (c) GWENDESIGN.

#include <M5Core2.h>
#include "M5Keyboard.h"
#include <Free_Fonts.h>

#define OFFSET_INPUT_Y (24)

#define KEYBOARD_X (2)
#define KEYBOARD_Y (32 + OFFSET_INPUT_Y )

#define KEY_W (45)
#define KEY_H (40)

#define COLS (7)
#define ROWS (4)

#define MAX_SHIFT_MODE (4)

#define COLOR_FRAME M5.Lcd.color565(124,163,121)
#define COLOR_TITLE M5.Lcd.color565(130,130,130)

const char keymap[MAX_SHIFT_MODE][ROWS][COLS] =
{
  {
	{'a', 'b', 'c', 'd', 'e', 'f', 'g'},
	{'h', 'i', 'j', 'k', 'l', 'm', 'n'},
	{'o', 'p', 'q', 'r', 's', 't', 'u'},
	{'v', 'w', 'x', 'y', 'z', ' ', '\002'}, // 002 = mode
  },
  {
	{'A', 'B', 'C', 'D', 'E', 'F', 'G'},
	{'H', 'I', 'J', 'K', 'L', 'M', 'N'},
	{'O', 'P', 'Q', 'R', 'S', 'T', 'U'},
	{'V', 'W', 'X', 'Y', 'Z', ' ', '\002'}, // 002 = mode
  },
  {
    {'7', '8', '9', '+', '[', ']', ' ', },
    {'4', '5', '6', '/', '\\', ' ', ' '},
    {'1', '2', '3', '-', '\'', ' ', ' '},
    {',', '0', '.', '=', ';', '`', '\002'}, // 002 = mode
/*
	{'`', '1', '2', '3', '4', '5', '6'},
	{'7', '8', '9', '0', '-', '=', '['},
	{']', '\\', ';', '\'', ',', '.', '/'},
	{' ', ' ', ' ', ' ', ' ', ' ', '\002'}, // 002 = mode
    */
  },
  {
	{'~', '!', '@', '#', '$', '%', '^'},
	{'&', '*', '(', ')', '_', '+', '{'},
	{'}', '|', ':', '"', '<', '>', '?'},
	{' ', ' ', ' ', ' ', ' ', ' ', '\002'}, // 002 = mode
  },
};

Button *_button_list[ROWS][COLS];
String _input_text = "";
String _old_input_text = "";
M5Keyboard::key_mode_t _key_mode = M5Keyboard::KEY_MODE_LETTER;
bool _keyboard_done = false;
bool _keyboard_cancel = false;
uint32_t _cursor_last;
bool _cursor_state = false;
ButtonColors _bc_on = {BLUE, GREEN, COLOR_FRAME};
ButtonColors _bc_off = {BLACK, GREEN, COLOR_FRAME};

bool M5Keyboard::Show(String& text, const char* title, key_mode_t keyMode )
{
    // wait till there is no touch anymore
    while (M5.Touch.ispressed())
        M5.update();

   M5.Lcd.pushState();
   M5.Lcd.clear(TFT_BLACK);

  _initKeyboard(text, title, keyMode);
  _drawKeyboard();
  _keyboard_done = false;
  _keyboard_cancel = false;
  while(_keyboard_done == false && _keyboard_cancel == false)
  {
    M5.update();

    // Blinking cursor
    if(millis() > _cursor_last)
    {
      _cursor_last = millis() + 500;
      _cursor_state = !_cursor_state;
      _updateInputText();
    }
  }
  while(M5.BtnB.isPressed())
  {
    M5.update();
  }
  while (M5.BtnC.isPressed())
  {
      M5.update();
  }
  _deinitKeyboard();
  text = _input_text;

  M5.Lcd.clear(TFT_BLACK);
  M5.Lcd.popState();

  return _keyboard_done ? true : false;
}

void M5Keyboard::_updateInputText()
{
  int oitw = M5.Lcd.textWidth(_old_input_text);
  int itw = M5.Lcd.textWidth(_input_text);

  // Hack to work around incorrect width returned by textWidth()
  //  when space char is at the end of input text
  if(_input_text.endsWith(" ") != 0)
  {
    itw += 14;
  }

  M5.Lcd.setFreeFont(FF2);
  if(_old_input_text != _input_text)
  {
    _old_input_text = _input_text;
    M5.Lcd.fillRect(0, OFFSET_INPUT_Y, max(oitw, itw) + 40, KEYBOARD_Y - OFFSET_INPUT_Y - 1, TFT_BLACK);
    M5.Lcd.setTextColor( TFT_WHITE );
    M5.Lcd.drawString(_input_text, 2, 10 + OFFSET_INPUT_Y);
  }
  else
  {
    if(_cursor_state == true)
    {
      M5.Lcd.fillRect(itw + 2 + 2, 2 + OFFSET_INPUT_Y, 15, KEYBOARD_Y - OFFSET_INPUT_Y - 12, COLOR_FRAME);
    }
    else
    {
      M5.Lcd.fillRect(itw + 2 + 2, 2 + OFFSET_INPUT_Y, 15, KEYBOARD_Y - OFFSET_INPUT_Y - 12, TFT_BLACK);
    }
  }
}

void M5Keyboard::_initKeyboard(String text, const char * title, key_mode_t keyMode)
{
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setFreeFont(FF1);
  if( title )
  {
      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(COLOR_TITLE, TFT_BLACK );
      M5.Lcd.drawString( title, 2, 4, 2);
  }
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);

  // Button A
  M5.Lcd.drawString("Del", 55, 226, 2);
  // Button B
  M5.Lcd.drawString("OK", 160, 226, 2);
  // Button C
  M5.Lcd.drawString("Cancel", 265, 226, 2);

  for(int r = 0; r < ROWS; r++)
  {
    for(int c = 0; c < COLS; c++)
    {
      _button_list[r][c] = new Button(0, 0, 0, 0, false, "", _bc_off, _bc_on);
      _button_list[r][c]->setTextSize(1);
    }
  }

  M5.Buttons.addHandler(_buttonEvent, E_TOUCH);
  M5.Buttons.addHandler(_btnAEvent, E_RELEASE);

  _input_text = text;
  _key_mode = keyMode;
}

void M5Keyboard::_deinitKeyboard()
{
  M5.Buttons.delHandlers(_buttonEvent, nullptr, nullptr);
  M5.Buttons.delHandlers(_btnAEvent, nullptr, nullptr);

  for(int r = 0; r < ROWS; r++)
  {
    for(int c = 0; c < COLS; c++)
    {
      delete(_button_list[r][c]);
      _button_list[r][c] = NULL;
    }
  }
}

void M5Keyboard::_drawKeyboard()
{
  int x, y;

  for(int r = 0; r < ROWS; r++)
  {
    for(int c = 0; c < COLS; c++)
    {
      x = (KEYBOARD_X + (c * KEY_W));
      y = (KEYBOARD_Y + (r * KEY_H));
      _button_list[r][c]->set(x, y, KEY_W, KEY_H);

      int key_page;
      switch (_key_mode)
      {
      case KEY_MODE_LETTER:      key_page = 0; break;
      case KEY_MODE_LETTERSHIFT: key_page = 1; break;
      case KEY_MODE_NUMBER:      key_page = 2; break;
      case KEY_MODE_SYM:         key_page = 3; break;
      }

      String key;
      char ch = keymap[key_page][r][c];

      if(ch == '\002')  // mode
      {
        _button_list[r][c]->setFreeFont(FF1);
        key = "Mode";
      }
      else
      {
        _button_list[r][c]->setFreeFont(FF3);
        key = String(ch);
      }
      _button_list[r][c]->setLabel(key.c_str());
      _button_list[r][c]->draw();
    }
  }
}

void M5Keyboard::_btnAEvent(Event& e)
{
  // Delete all (long press) or delete one char (short press)
  if(e.button == &M5.BtnA)
  {
    if(e.duration > 500)
    {
      _input_text = "";
    }
    else
    {
      _input_text = _input_text.substring(0, _input_text.length() - 1);
    }
    _updateInputText();
  }
}

void M5Keyboard::_buttonEvent(Event& e)
{
  Button& b = *e.button;

  // Delete
  if(e.button == &M5.BtnA)
  {
      // Ignore - handled in btnAEvent()
      return;
  }
  // Done
  else if(e.button == &M5.BtnB)
  {
      _keyboard_done = true;
      return;
  }
  // Cancel
  else if(e.button == &M5.BtnC)
  {
      _keyboard_cancel = true;
      return;
  }
  else if(e.button == &M5.background)
  {
    // Ignore default background button
    return;
  }
  else
  {
    if(String(b.label()) == "Mode")
    {
        int key_mode = _key_mode;
        key_mode++;
        key_mode = key_mode % NUM_KEY_MODES;
        _key_mode = (key_mode_t)key_mode;
        M5Keyboard::_drawKeyboard();
      return;
    }
    _input_text += b.label();
  }
  _updateInputText();
}
