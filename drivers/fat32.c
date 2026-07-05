// drivers/fat32.c

#include <fat32.h>
#include <ata.h>
#include <video.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t  jump[3];
    uint8_t  oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t first_cluster_high;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;
#pragma pack(pop)

static fat32_boot_sector_t boot_sector;
static uint32_t bytes_per_sector;
static uint32_t sectors_per_cluster;
static uint32_t reserved_sectors;
static uint32_t fat_size;
static uint32_t root_cluster;
static uint32_t data_start_lba;

static int read_cluster(uint32_t cluster, uint8_t* buffer) {
    uint32_t lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        if (ata_read_sector(lba + i, buffer + i * bytes_per_sector) != 0) {
            return -1;
        }
    }
    return 0;
}

static uint32_t next_cluster(uint32_t cluster) {
    uint32_t fat_entry_lba = reserved_sectors + (cluster * 4) / bytes_per_sector;
    uint32_t offset = (cluster * 4) % bytes_per_sector;
    uint8_t sector[512];
    if (ata_read_sector(fat_entry_lba, sector) != 0) return 0x0FFFFFF7;
    uint32_t entry = *(uint32_t*)(sector + offset);
    return entry & 0x0FFFFFFF;
}

void fat32_init(void) {
    uint8_t sector[512];
    if (ata_read_sector(0, sector) != 0) {
        print_string("FAT32: Failed to read boot sector\n", COLOR_RED);
        return;
    }
    fat32_boot_sector_t* bs = (fat32_boot_sector_t*)sector;
    boot_sector = *bs;

    bytes_per_sector = bs->bytes_per_sector;
    sectors_per_cluster = bs->sectors_per_cluster;
    reserved_sectors = bs->reserved_sectors;
    fat_size = bs->fat_size_32;
    root_cluster = bs->root_cluster;

    uint32_t fat_start_lba = reserved_sectors;
    data_start_lba = fat_start_lba + fat_size * bs->num_fats;

    print_string("FAT32: Boot sector read OK. Root cluster: ", COLOR_GREEN);
    print_int(root_cluster, COLOR_GREEN);
    print_string("\n", COLOR_GREEN);
}

void fat32_ls(void) {
    uint8_t cluster_buffer[512 * 64];
    if (read_cluster(root_cluster, cluster_buffer) != 0) {
        print_string("FAT32: Failed to read root directory\n", COLOR_RED);
        return;
    }

    uint32_t entries_per_cluster = (sectors_per_cluster * bytes_per_sector) / sizeof(fat32_dir_entry_t);
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_buffer;

    int found = 0;
    for (uint32_t i = 0; i < entries_per_cluster; i++) {
        fat32_dir_entry_t* entry = &entries[i];
        if (entry->name[0] == 0x00) break;
        if (entry->name[0] == 0xE5) continue;
        if (entry->attr & 0x0F) continue;

        char filename[13];
        int idx = 0;
        for (int j = 0; j < 8; j++) {
            if (entry->name[j] != ' ') filename[idx++] = entry->name[j];
        }
        if (entry->name[8] != ' ') {
            filename[idx++] = '.';
            for (int j = 8; j < 11; j++) {
                if (entry->name[j] != ' ') filename[idx++] = entry->name[j];
            }
        }
        filename[idx] = '\0';

        print_string(filename, COLOR_WHITE);
        if (entry->attr & 0x10) print_string("/", COLOR_CYAN);
        print_string("  ", COLOR_WHITE);
        print_int(entry->file_size, COLOR_WHITE);
        print_string(" bytes\n", COLOR_WHITE);
        found = 1;
    }
    if (!found) print_string("(empty directory)\n", COLOR_GRAY);
}

void fat32_cat(const char* filename) {
    // Преобразуем в 8.3
    char short_name[11];
    for (int i = 0; i < 11; i++) short_name[i] = ' ';
    int i, j;
    for (i = 0; filename[i] && filename[i] != '.' && i < 8; i++) {
        short_name[i] = filename[i];
    }
    if (filename[i] == '.') {
        i++;
        for (j = 0; filename[i] && j < 3; i++, j++) {
            short_name[8 + j] = filename[i];
        }
    }
    for (i = 0; i < 11; i++) {
        if (short_name[i] >= 'a' && short_name[i] <= 'z')
            short_name[i] = short_name[i] - 'a' + 'A';
    }

    uint8_t cluster_buffer[512 * 64];
    if (read_cluster(root_cluster, cluster_buffer) != 0) {
        print_string("FAT32: Failed to read root directory\n", COLOR_RED);
        return;
    }

    uint32_t entries_per_cluster = (sectors_per_cluster * bytes_per_sector) / sizeof(fat32_dir_entry_t);
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_buffer;
    fat32_dir_entry_t* found_entry = NULL;

    for (uint32_t i = 0; i < entries_per_cluster; i++) {
        fat32_dir_entry_t* entry = &entries[i];
        if (entry->name[0] == 0x00) break;
        if (entry->name[0] == 0xE5) continue;
        if (entry->attr & 0x0F) continue;
        int match = 1;
        for (int k = 0; k < 11; k++) {
            if (entry->name[k] != short_name[k]) { match = 0; break; }
        }
        if (match) { found_entry = entry; break; }
    }

    if (!found_entry) {
        print_string("File not found: ", COLOR_RED);
        print_string(filename, COLOR_RED);
        print_string("\n", COLOR_RED);
        return;
    }

    uint32_t cluster = found_entry->first_cluster_low | (found_entry->first_cluster_high << 16);
    uint32_t bytes_remaining = found_entry->file_size;

    while (bytes_remaining > 0 && cluster < 0x0FFFFFF8) {
        uint8_t data_buffer[512 * 8];
        uint32_t bytes_to_read = (bytes_remaining < sectors_per_cluster * bytes_per_sector) ?
                                 bytes_remaining : sectors_per_cluster * bytes_per_sector;
        uint32_t lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
        uint32_t remaining = bytes_to_read;
        uint32_t offset = 0;
        while (remaining > 0) {
            uint32_t chunk = (remaining < 512) ? remaining : 512;
            if (ata_read_sector(lba, data_buffer + offset) != 0) {
                print_string("FAT32: Error reading file data\n", COLOR_RED);
                return;
            }
            remaining -= chunk;
            offset += chunk;
            lba++;
        }
        for (uint32_t i = 0; i < bytes_to_read; i++) {
            char c = data_buffer[i];
            if (c >= 0x20 && c <= 0x7E) print_char(c, COLOR_WHITE);
            else if (c == '\n' || c == '\r' || c == '\t') print_char(c, COLOR_WHITE);
            else print_char('.', COLOR_WHITE);
        }
        bytes_remaining -= bytes_to_read;
        cluster = next_cluster(cluster);
    }
    print_char('\n', COLOR_WHITE);
}