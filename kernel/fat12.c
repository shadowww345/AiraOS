
#include <kernel.h>
#include <graphics.h>
#include <sound.h>
#include <memory.h>
#include <fat12.h>

unsigned short inw(unsigned short port) {
    unsigned short result;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void read_sector(uint32_t lba, uint8_t *buffer) {
    while (inb(0x1F7) & 0x80);

    outb(0x1F6, (0xE0 | ((lba >> 24) & 0x0F)));
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);
    for (int i = 0; i < 1000; i++) {
        uint8_t status = inb(0x1F7);
        if (!(status & 0x80) && (status & 0x08)) {
            for (int j = 0; j < 256; j++) {
                uint16_t data = inw(0x1F0);
                buffer[j * 2] = (uint8_t)data;
                buffer[j * 2 + 1] = (uint8_t)(data >> 8);
            }
            return; 
        }
        if (status & 0x01) { 
        	print("Read Failed");
            return;
        }
    }
}

uint32_t cluster_to_lba(uint16_t cluster, struct FAT12_BPB *bpb) {
    uint32_t root_dir_sectors = ((bpb->root_entries * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
    uint32_t data_region_start = bpb->reserved_sectors + (bpb->fat_count * bpb->sectors_per_fat) + root_dir_sectors;
    return data_region_start + (cluster - 2) * bpb->sectors_per_cluster;
}


uint16_t get_next_cluster(uint16_t current, uint8_t *fat_table) {
    uint32_t offset = current + (current / 2);
    uint16_t entry = *(uint16_t *)(fat_table + offset);

    if (current % 2 == 0) {
        return entry & 0x0FFF;
    } else {
        return entry >> 4;
    }
}


void load_file(const char* fat_name, uint8_t* load_address, struct FAT12_BPB* bpb) {
    uint8_t sector_buffer[512];
    struct FAT12_DirectoryEntry* entry = 0;
    for (uint32_t i = 0; i < 14; i++) {
        read_sector(19 + i, sector_buffer);
        
        for (uint32_t j = 0; j < 512; j += 32) {
            struct FAT12_DirectoryEntry* temp = (struct FAT12_DirectoryEntry*)(sector_buffer + j);
            if (memcmp(temp->filename, fat_name, 11) == 0) {
                entry = temp;
                break;
            }
        }
        if (entry) break;
    }

    if (!entry) return;
    uint8_t fat_table[512 * 9];
    for (uint32_t i = 0; i < 9; i++) {
        read_sector(1 + i, fat_table + (i * 512));
    }
    uint16_t current_cluster = entry->first_cluster;
    
    while (current_cluster < 0x0FF8) {
        uint32_t lba = 33 + (current_cluster - 2);
        
        read_sector(lba, load_address);
        load_address += 512;
        uint32_t fat_offset = current_cluster + (current_cluster / 2);
        uint16_t next = *(uint16_t*)(fat_table + fat_offset);

        if (current_cluster % 2 == 0) 
            current_cluster = next & 0x0FFF;
        else 
            current_cluster = next >> 4;
    }
}

void list_root_directory() {
    struct FAT12_DirectoryEntry entries[16];
    read_sector(19, (uint8_t*)entries);
    for(int i = 0; i < 16; i++) {
        if (entries[i].filename[0] == 0) break;
        if (entries[i].filename[0] == 0xE5) continue;
        print(entries[i].filename);
        uint16_t cluster = *(uint16_t*)(entries + 27);
        print(" - Cluster: ");
        print_int(cluster);
        print("\n");
        print("\n"); 
    }
}

void cat_file(const char* target_name) {
    uint8_t buffer[512];
    struct FAT12_DirectoryEntry* entry = 0;
    read_sector(19, buffer); 
    for (int i = 0; i < 16; i++) {
        struct FAT12_DirectoryEntry* temp = (struct FAT12_DirectoryEntry*)(buffer + (i * 32));
        if (memcmp(temp->filename, target_name, 11) == 0) {
            entry = temp;
            break;
        }
    }

    if (!entry) {
        print("\nError: File not found!\n");
        return;
    }
    uint16_t current_cluster = entry->first_cluster;
    uint32_t lba = 33 + (current_cluster - 2);
    uint8_t file_content[512];
    read_sector(lba, file_content);
    for(uint32_t i = 0; i < entry->file_size; i++) {
        put_char(file_content[i]); 
    }
    print("\n");
}