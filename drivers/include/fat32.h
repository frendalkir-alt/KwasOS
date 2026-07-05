// drivers/include/fat32.h

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

// Инициализация FAT32 (читает загрузочный сектор)
void fat32_init(void);

// Вывод списка файлов в корневом каталоге
void fat32_ls(void);

void fat32_cat(const char* filename);

#endif