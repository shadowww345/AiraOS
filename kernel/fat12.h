#ifndef FAT12_H
#define FAT12_H

typedef struct FAT12_BPB {
    uint8_t  jmp[3];
    char     oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors;
    uint8_t  media_type;
    uint16_t sectors_per_fat;
} __attribute__((packed)) FAT12_BPB;

typedef struct FAT12_DirectoryEntry {
    uint8_t  filename[8];
    uint8_t  extension[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  created_time_ms;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t last_access_date;
    uint16_t ignore_in_fat12;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster;
    uint32_t file_size;
} __attribute__((packed)) DirectoryEntry;

unsigned short inw(unsigned short port);
void read_sector(uint32_t lba, uint8_t *buffer);
uint32_t cluster_to_lba(uint16_t cluster, struct FAT12_BPB *bpb);
uint16_t get_next_cluster(uint16_t current, uint8_t *fat_table);
void load_file(const char* fat_name, uint8_t* load_address, FAT12_BPB* bpb);
void list_root_directory();
void cat_file(const char* target_name);


#endif