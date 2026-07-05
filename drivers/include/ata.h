// drivers/include/ata.h

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

void ata_init(void);

int ata_read_sector(uint32_t lba, uint8_t* buffer);

#endif