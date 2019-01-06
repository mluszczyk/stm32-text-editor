#ifndef _KEYBOARD_H
#define _KEYBOARD_H 1

// To be implemented by the user:
void ButtonPressed(int row_pressed, int col_pressed);
void FixButton(void);
void AmbiguousPress(void);

// Delivered by keyboard.c:
void KeyboardConfigure(void);

#endif
