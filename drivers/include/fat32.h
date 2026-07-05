// drivers/include/fat32.h

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

void fat32_init(void);

void fat32_ls(void);

void fat32_cat(const char* filename);

#endif