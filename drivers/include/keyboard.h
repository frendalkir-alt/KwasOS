// drivers/include/keyboard.h

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_UP    0x80
#define KEY_DOWN  0x81
#define KEY_LEFT  0x82
#define KEY_RIGHT 0x83

void init_keyboard(void);
void keyboard_handler(void);
int get_key(void);
int kbhit(void);
void clear_keyboard_buffer(void);

#endif