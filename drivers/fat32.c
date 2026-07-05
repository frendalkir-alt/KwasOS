// drivers/fat32.c

#include <fat32.h>
#include <ata.h>
#include <video.h>
#include <string.h>
#include <stdint.h>

// ============================================================
//  Структуры FAT32
// ============================================================
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

// ============================================================
//  Глобальные переменные
// ============================================================
static fat32_boot_sector_t boot_sector;
static uint32_t bytes_per_sector;
static uint32_t sectors_per_cluster;
static uint32_t reserved_sectors;
static uint32_t fat_size;
static uint32_t root_cluster;
static uint32_t data_start_lba;

// Большой буфер для работы с кластерами (1 МБ)
static uint8_t cluster_buffer[1024 * 1024];

// ============================================================
//  Вспомогательные функции работы с кластерами
// ============================================================
static int read_cluster(uint32_t cluster, uint8_t* buffer) {
    uint32_t lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        if (ata_read_sector(lba + i, buffer + i * bytes_per_sector) != 0) {
            return -1;
        }
    }
    return 0;
}

static int write_cluster(uint32_t cluster, const uint8_t* buffer) {
    uint32_t lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        if (ata_write_sector(lba + i, buffer + i * bytes_per_sector) != 0) {
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

// ============================================================
//  Инициализация FAT32
// ============================================================
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

    data_start_lba = reserved_sectors + fat_size * bs->num_fats;

    print_string("FAT32: Boot sector read OK. Root cluster: ", COLOR_GREEN);
    print_int(root_cluster, COLOR_GREEN);
    print_string("\n", COLOR_GREEN);
}

// ============================================================
//  Получение размера диска через ATA IDENTIFY
// ============================================================
uint64_t fat32_get_disk_size(void) {
    uint16_t identify[256];
    if (ata_identify(identify) != 0) return 0;
    uint64_t sectors = 0;
    if (identify[83] & (1 << 10)) { // LBA48 support
        sectors = *(uint64_t*)(identify + 100);
    } else {
        sectors = identify[60] | ((uint64_t)identify[61] << 16);
    }
    return sectors;
}

// ============================================================
//  Форматирование диска в FAT32 (с автоматическим подбором параметров)
// ============================================================
void fat32_format(void) {
    uint64_t total_sectors = fat32_get_disk_size();
    if (total_sectors == 0) {
        print_string("FAT32: Failed to detect disk size\n", COLOR_RED);
        return;
    }
    print_string("FAT32: Disk size = ", COLOR_WHITE);
    print_int(total_sectors, COLOR_WHITE);
    print_string(" sectors (", COLOR_WHITE);
    print_int(total_sectors / 2 / 1024, COLOR_WHITE);
    print_string(" MB)\n", COLOR_WHITE);

    uint32_t bytes_per_sector = 512;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors = 32;
    uint32_t num_fats = 2;
    uint32_t root_cluster = 2;
    uint32_t fat_size;

    // Определяем размер кластера в зависимости от размера диска
    if (total_sectors <= 4096) { // до 2 МБ
        sectors_per_cluster = 1;
    } else if (total_sectors <= 16384) { // до 8 МБ
        sectors_per_cluster = 4;
    } else if (total_sectors <= 65536) { // до 32 МБ
        sectors_per_cluster = 8;
    } else if (total_sectors <= 262144) { // до 128 МБ
        sectors_per_cluster = 16;
    } else if (total_sectors <= 1048576) { // до 512 МБ
        sectors_per_cluster = 32;
    } else if (total_sectors <= 4194304) { // до 2 ГБ
        sectors_per_cluster = 64;
    } else if (total_sectors <= 16777216) { // до 8 ГБ
        sectors_per_cluster = 128;
    } else if (total_sectors <= 67108864) { // до 32 ГБ
        sectors_per_cluster = 256;
    } else if (total_sectors <= 268435456) { // до 128 ГБ
        sectors_per_cluster = 512;
    } else { // до 512 ГБ и более
        sectors_per_cluster = 1024;
    }

    // Количество кластеров
    uint64_t data_sectors = total_sectors - reserved_sectors;
    uint64_t total_clusters = data_sectors / sectors_per_cluster;
    if (total_clusters < 65525) {
        while (sectors_per_cluster > 1 && total_clusters < 65525) {
            sectors_per_cluster /= 2;
            total_clusters = data_sectors / sectors_per_cluster;
        }
    }

    // Размер FAT (в секторах)
    uint32_t fat_bytes = (uint32_t)(total_clusters * 4 + 512);
    fat_size = (fat_bytes + bytes_per_sector - 1) / bytes_per_sector;
    if (fat_size < 1) fat_size = 1;

    if (reserved_sectors + num_fats * fat_size >= total_sectors) {
        print_string("FAT32: Disk too small for FAT32\n", COLOR_RED);
        return;
    }

    // Заполняем загрузочный сектор
    uint8_t boot_sector[512];
    for (int i = 0; i < 512; i++) boot_sector[i] = 0;

    fat32_boot_sector_t* bs = (fat32_boot_sector_t*)boot_sector;
    bs->jump[0] = 0xEB; bs->jump[1] = 0x3C; bs->jump[2] = 0x90;
    const char* oem = "KwasFS  ";
    for (int i = 0; i < 8; i++) bs->oem[i] = oem[i];
    bs->bytes_per_sector = bytes_per_sector;
    bs->sectors_per_cluster = sectors_per_cluster;
    bs->reserved_sectors = reserved_sectors;
    bs->num_fats = num_fats;
    bs->root_entries = 0;
    bs->total_sectors_16 = 0;
    bs->media = 0xF8;
    bs->fat_size_16 = 0;
    bs->sectors_per_track = 63;
    bs->num_heads = 255;
    bs->hidden_sectors = 0;
    bs->total_sectors_32 = (total_sectors < 0xFFFFFFFF) ? (uint32_t)total_sectors : 0xFFFFFFFF;
    bs->fat_size_32 = fat_size;
    bs->ext_flags = 0;
    bs->fs_version = 0;
    bs->root_cluster = root_cluster;
    bs->fs_info = 1;
    bs->backup_boot_sector = 6;
    for (int i = 0; i < 12; i++) bs->reserved[i] = 0;
    bs->drive_number = 0x80;
    bs->reserved1 = 0;
    bs->boot_signature = 0x29;
    bs->volume_id = 0x12345678;
    const char* label = "KWASOSDISK";
    for (int i = 0; i < 11; i++) bs->volume_label[i] = (i < 11) ? label[i] : ' ';
    const char* fstype = "FAT32   ";
    for (int i = 0; i < 8; i++) bs->fs_type[i] = fstype[i];
    boot_sector[510] = 0x55;
    boot_sector[511] = 0xAA;

    if (ata_write_sector(0, boot_sector) != 0) {
        print_string("FAT32: Failed to write boot sector\n", COLOR_RED);
        return;
    }

    // Обнуляем FAT
    uint8_t fat_sector[512];
    for (int i = 0; i < 512; i++) fat_sector[i] = 0;
    fat_sector[0] = 0xF8; fat_sector[1] = 0xFF; fat_sector[2] = 0xFF; fat_sector[3] = 0x0F;
    *(uint32_t*)(fat_sector + 8) = 0x0FFFFFF8;

    uint32_t fat_start = reserved_sectors;
    for (uint32_t fat = 0; fat < num_fats; fat++) {
        for (uint32_t sec = 0; sec < fat_size; sec++) {
            if (ata_write_sector(fat_start + fat * fat_size + sec, fat_sector) != 0) {
                print_string("FAT32: Failed to write FAT\n", COLOR_RED);
                return;
            }
        }
    }

    // Очищаем корневой каталог (кластер 2)
    uint32_t root_lba = reserved_sectors + num_fats * fat_size +
                       (root_cluster - 2) * sectors_per_cluster;
    uint8_t empty_sector[512];
    for (int i = 0; i < 512; i++) empty_sector[i] = 0;
    empty_sector[0] = 0x00; // признак конца каталога
    for (uint32_t sec = 0; sec < sectors_per_cluster; sec++) {
        if (ata_write_sector(root_lba + sec, empty_sector) != 0) {
            print_string("FAT32: Failed to clear root directory\n", COLOR_RED);
            return;
        }
    }

    // Перечитываем загрузочный сектор
    fat32_init();

    print_string("FAT32: Disk formatted successfully\n", COLOR_GREEN);
    print_string("FAT32: Clusters: ", COLOR_WHITE);
    print_int(total_clusters, COLOR_WHITE);
    print_string(", Cluster size: ", COLOR_WHITE);
    print_int(sectors_per_cluster * bytes_per_sector, COLOR_WHITE);
    print_string(" bytes\n", COLOR_WHITE);
}

// ============================================================
//  Вывод списка файлов (ls)
// ============================================================
void fat32_ls(void) {
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

// ============================================================
//  Вывод содержимого файла (cat)
// ============================================================
void fat32_cat(const char* filename) {
    // Преобразуем имя в 8.3
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

    // Читаем корневой каталог
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
        uint8_t data_buffer[512]; // временный секторный буфер
        uint32_t lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
        uint32_t bytes_in_cluster = (bytes_remaining < sectors_per_cluster * bytes_per_sector) ?
                                    bytes_remaining : sectors_per_cluster * bytes_per_sector;
        uint32_t remaining = bytes_in_cluster;
        uint32_t offset = 0;
        while (remaining > 0) {
            uint32_t chunk = (remaining < 512) ? remaining : 512;
            if (ata_read_sector(lba, data_buffer) != 0) {
                print_string("FAT32: Error reading file data\n", COLOR_RED);
                return;
            }
            for (uint32_t i = 0; i < chunk; i++) {
                char c = data_buffer[i];
                if (c >= 0x20 && c <= 0x7E) print_char(c, COLOR_WHITE);
                else if (c == '\n' || c == '\r' || c == '\t') print_char(c, COLOR_WHITE);
                else print_char('.', COLOR_WHITE);
            }
            remaining -= chunk;
            offset += chunk;
            lba++;
        }
        bytes_remaining -= bytes_in_cluster;
        cluster = next_cluster(cluster);
    }
    print_char('\n', COLOR_WHITE);
}

// ============================================================
//  Запись файла (echo >)
// ============================================================
static uint32_t fat32_find_free_cluster(void) {
    uint8_t sector[512];
    for (uint32_t cluster = 2; cluster < 0x0FFFFFF8; cluster++) {
        uint32_t fat_lba = reserved_sectors + (cluster * 4) / bytes_per_sector;
        uint32_t offset = (cluster * 4) % bytes_per_sector;
        if (ata_read_sector(fat_lba, sector) != 0) return 0;
        uint32_t entry = *(uint32_t*)(sector + offset);
        if ((entry & 0x0FFFFFFF) == 0) {
            return cluster;
        }
    }
    return 0;
}

static int fat32_write_file_data(uint32_t start_cluster, const uint8_t* data, uint32_t size) {
    uint32_t cluster = start_cluster;
    uint32_t bytes_written = 0;
    uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;

    while (bytes_written < size) {
        uint32_t lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
        uint32_t bytes_to_write = (size - bytes_written > cluster_size) ? cluster_size : (size - bytes_written);
        uint32_t sectors_to_write = (bytes_to_write + bytes_per_sector - 1) / bytes_per_sector;

        for (uint32_t i = 0; i < sectors_to_write; i++) {
            uint32_t offset = i * bytes_per_sector;
            if (ata_write_sector(lba + i, data + bytes_written + offset) != 0) {
                return -1;
            }
        }

        bytes_written += bytes_to_write;

        if (bytes_written < size) {
            uint32_t next_cluster = fat32_find_free_cluster();
            if (next_cluster == 0) return -1;
            uint32_t fat_lba = reserved_sectors + (cluster * 4) / bytes_per_sector;
            uint32_t offset = (cluster * 4) % bytes_per_sector;
            uint8_t sector[512];
            if (ata_read_sector(fat_lba, sector) != 0) return -1;
            *(uint32_t*)(sector + offset) = next_cluster;
            if (ata_write_sector(fat_lba, sector) != 0) return -1;

            cluster = next_cluster;
            fat_lba = reserved_sectors + (cluster * 4) / bytes_per_sector;
            offset = (cluster * 4) % bytes_per_sector;
            if (ata_read_sector(fat_lba, sector) != 0) return -1;
            *(uint32_t*)(sector + offset) = 0x0FFFFFF8;
            if (ata_write_sector(fat_lba, sector) != 0) return -1;
        }
    }
    return 0;
}

void fat32_write_file(const char* filename, const char* content) {
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

    if (read_cluster(root_cluster, cluster_buffer) != 0) {
        print_string("FAT32: Failed to read root directory\n", COLOR_RED);
        return;
    }

    uint32_t entries_per_cluster = (sectors_per_cluster * bytes_per_sector) / sizeof(fat32_dir_entry_t);
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)cluster_buffer;
    fat32_dir_entry_t* target_entry = NULL;
    int entry_index = -1;

    for (uint32_t i = 0; i < entries_per_cluster; i++) {
        fat32_dir_entry_t* entry = &entries[i];
        if (entry->name[0] == 0x00) {
            if (!target_entry) { target_entry = entry; entry_index = i; }
            break;
        }
        if (entry->name[0] == 0xE5) {
            if (!target_entry) { target_entry = entry; entry_index = i; }
            continue;
        }
        if (entry->attr & 0x0F) continue;
        int match = 1;
        for (int k = 0; k < 11; k++) {
            if (entry->name[k] != short_name[k]) { match = 0; break; }
        }
        if (match) {
            target_entry = entry;
            entry_index = i;
            break;
        }
    }

    if (!target_entry) {
        print_string("FAT32: No free directory entry (use command format to write files)\n", COLOR_RED);
        return;
    }

    uint32_t start_cluster = fat32_find_free_cluster();
    if (start_cluster == 0) {
        print_string("FAT32: No free clusters\n", COLOR_RED);
        return;
    }

    uint32_t fat_lba = reserved_sectors + (start_cluster * 4) / bytes_per_sector;
    uint32_t offset = (start_cluster * 4) % bytes_per_sector;
    uint8_t sector[512];
    if (ata_read_sector(fat_lba, sector) != 0) {
        print_string("FAT32: Failed to read FAT\n", COLOR_RED);
        return;
    }
    *(uint32_t*)(sector + offset) = 0x0FFFFFF8;
    if (ata_write_sector(fat_lba, sector) != 0) {
        print_string("FAT32: Failed to write FAT\n", COLOR_RED);
        return;
    }

    uint32_t content_len = 0;
    while (content[content_len]) content_len++;
    if (fat32_write_file_data(start_cluster, (const uint8_t*)content, content_len) != 0) {
        print_string("FAT32: Failed to write file data\n", COLOR_RED);
        return;
    }

    for (int k = 0; k < 11; k++) target_entry->name[k] = short_name[k];
    target_entry->attr = 0x20;
    target_entry->nt_reserved = 0;
    target_entry->create_time_tenth = 0;
    target_entry->create_time = 0;
    target_entry->create_date = 0;
    target_entry->access_date = 0;
    target_entry->first_cluster_high = (start_cluster >> 16) & 0xFFFF;
    target_entry->modify_time = 0;
    target_entry->modify_date = 0;
    target_entry->first_cluster_low = start_cluster & 0xFFFF;
    target_entry->file_size = content_len;

    if (write_cluster(root_cluster, cluster_buffer) != 0) {
        print_string("FAT32: Failed to write directory\n", COLOR_RED);
        return;
    }

    print_string("File written: ", COLOR_GREEN);
    print_string(filename, COLOR_GREEN);
    print_string("\n", COLOR_GREEN);
}

void fat32_delete_file(const char* filename) {
    // Преобразуем имя в 8.3
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

    // Читаем корневой каталог
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

    // Помечаем запись как удалённую (0xE5)
    found_entry->name[0] = 0xE5;

    // Записываем изменённый кластер каталога
    if (write_cluster(root_cluster, cluster_buffer) != 0) {
        print_string("FAT32: Failed to write directory\n", COLOR_RED);
        return;
    }

    print_string("File deleted: ", COLOR_GREEN);
    print_string(filename, COLOR_GREEN);
    print_string("\n", COLOR_GREEN);
}