#ifndef KEYBOARD_H
#define KEYBOARD_H

// Возвращает ASCII-код (0-127) или специальные коды > 255
int get_key();

// Специальные коды для клавиш, не имеющих ASCII
#define KEY_UP    256
#define KEY_DOWN  257
#define KEY_LEFT  258
#define KEY_RIGHT 259

#endif