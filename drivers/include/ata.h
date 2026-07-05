// drivers/include/ata.h

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Инициализация ATA (Primary Master)
void ata_init(void);

// Чтение одного сектора (512 байт) с LBA-адреса
int ata_read_sector(uint32_t lba, uint8_t* buffer);

#endif