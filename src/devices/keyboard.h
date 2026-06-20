#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Инициализация буфера клавиатуры
void keyboard_init(void);

// Положить символ в буфер (вызывается из GUI при нажатии клавиши)
void keyboard_put(uint8_t key);

// Прочитать символ из буфера (для эмуляции IN port 0)
// Возвращает 0, если буфер пуст (или символ 0)
uint8_t keyboard_read(void);

// Проверить, есть ли данные в буфере
bool keyboard_has_data(void);

#endif