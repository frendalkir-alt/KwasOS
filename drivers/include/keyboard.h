#ifndef KEYBOARD_H
#define KEYBOARD_H

int get_key(void);
void keyboard_handler(void);
void init_keyboard(void);

#define KEY_UP    256
#define KEY_DOWN  257
#define KEY_LEFT  258
#define KEY_RIGHT 259

#endif