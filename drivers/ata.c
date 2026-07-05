// drivers/ata.c

#include <ata.h>
#include <io.h>
#include <video.h>
#include <stdint.h>

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_REG_DATA       0x1F0
#define ATA_REG_ERROR      0x1F1
#define ATA_REG_SECT_COUNT 0x1F2
#define ATA_REG_LBA_LOW    0x1F3
#define ATA_REG_LBA_MID    0x1F4
#define ATA_REG_LBA_HIGH   0x1F5
#define ATA_REG_DRIVE_HEAD 0x1F6
#define ATA_REG_STATUS     0x1F7
#define ATA_REG_COMMAND    0x1F7

#define ATA_CMD_READ_PIO   0x20

static int ata_wait_ready(void) {
    uint8_t status;
    for (int i = 0; i < 100000; i++) {
        status = inb(ATA_REG_STATUS);
        if ((status & 0x80) == 0) return 1;
    }
    return 0;
}

void ata_init(void) {
    if (ata_wait_ready()) {
        print_string("ATA: Primary master detected\n", COLOR_GREEN);
    } else {
        print_string("ATA: Primary master not ready\n", COLOR_RED);
    }
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!ata_wait_ready()) return -1;

    outb(ATA_REG_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    outb(ATA_REG_SECT_COUNT, 1);
    outb(ATA_REG_LBA_LOW, lba & 0xFF);
    outb(ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);

    outb(ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(ATA_REG_STATUS);
        if (status & 0x08) break;
        if (status & 0x01) return -1;
    }

    for (int i = 0; i < 256; i++) {
        uint16_t word = inw(ATA_REG_DATA);
        buffer[i*2] = word & 0xFF;
        buffer[i*2+1] = (word >> 8) & 0xFF;
    }
    return 0;
}