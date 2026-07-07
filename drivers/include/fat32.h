// drivers/include/fat32.h

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

void fat32_init(void);
void fat32_ls(void);
void fat32_cat(const char* filename);
void fat32_write_file(const char* filename, const char* content);
void fat32_delete_file(const char* filename);
void fat32_format(void);
void fat32_mkdir(const char* dirname);
int fat32_cd(const char* dirname);
void fat32_pwd(void);

uint64_t fat32_get_disk_size(void);

#endif