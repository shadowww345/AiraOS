#ifndef FAT32_H
#define FAT32_H

#include <sound.h>

typedef struct FAT32_BPB {
    uint8_t  jmp[3];
    char     oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved0[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} __attribute__((packed)) FAT32_BPB;

typedef struct FAT32_DirectoryEntry {
    uint8_t  filename[8];
    uint8_t  extension[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  created_time_ms;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) FAT32_DirectoryEntry;

typedef struct FAT32_LFN_Entry {
    uint8_t  order;
    uint16_t name1[5];
    uint8_t  attributes;
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];
    uint16_t first_cluster_zero;
    uint16_t name3[2];
} __attribute__((packed)) FAT32_LFN_Entry;

#define FAT32_ATTR_LFN        0x0F
#define FAT32_ATTR_DIRECTORY  0x10
#define FAT32_ATTR_VOLUME_ID  0x08
#define FAT32_ATTR_ARCHIVE    0x20
#define FAT32_EOC_MIN         0x0FFFFFF8u
#define FAT32_BAD_CLUSTER     0x0FFFFFF7u
#define FAT32_FREE_CLUSTER    0x00000000u

int fat32_init(uint32_t partition_lba);

uint32_t fat32_cluster_to_lba(uint32_t cluster);
uint32_t fat32_get_next_cluster(uint32_t cluster);

void fat32_read_cluster(uint32_t cluster, uint8_t* buffer);

void fat32_list_directory(uint32_t dir_cluster);
void fat32_list_root_directory();
void fat32_list_current_directory();

int fat32_find_in_dir(uint32_t dir_cluster, const char* name, FAT32_DirectoryEntry* out_entry);
int fat32_find(const char* name, FAT32_DirectoryEntry* out_entry);

uint32_t fat32_load_file(const char* name, uint8_t* load_address);

void fat32_cat_file(const char* name);

int fat32_write_file(const char* name, const uint8_t* data, uint32_t size);

int fat32_make_directory(const char* name);

int fat32_change_directory(const char* name);

uint32_t fat32_get_current_dir_cluster();

#endif
