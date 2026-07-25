#include <kernel.h>
#include <graphics.h>
#include <sound.h>
#include <memory.h>
#include <fat32.h>

extern void read_sector(uint32_t lba, uint8_t *buffer);
extern int  write_sector(uint32_t lba, const uint8_t *buffer);

#define FAT32_MAX_CLUSTER_BYTES 0x8000

static uint32_t g_partition_lba;
static uint32_t g_fat_start_lba;
static uint32_t g_data_start_lba;
static uint32_t g_root_cluster;
static uint32_t g_sectors_per_cluster;
static uint32_t g_bytes_per_sector;
static uint32_t g_cluster_size;
static uint32_t g_fat_count;
static uint32_t g_sectors_per_fat32;
static uint32_t g_total_clusters;
static uint8_t  g_mounted = 0;

static uint32_t g_current_dir_cluster;

static uint32_t g_fat_cache_sector = 0xFFFFFFFF;
static uint8_t  g_fat_cache[512];

static uint8_t g_cluster_buf[FAT32_MAX_CLUSTER_BYTES];

static uint32_t g_alloc_hint = 2;

static int compare_string_local(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] == s2[i]) {
        if (s1[i] == '\0') return 1;
        i++;
    }
    return 0;
}

static int str_eq_ci(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == *b;
}

static void format_short_name(FAT32_DirectoryEntry* e, char* out) {
    int pos = 0;
    for (int i = 0; i < 8 && e->filename[i] != ' '; i++) out[pos++] = (char)e->filename[i];
    if (e->extension[0] != ' ') {
        out[pos++] = '.';
        for (int i = 0; i < 3 && e->extension[i] != ' '; i++) out[pos++] = (char)e->extension[i];
    }
    out[pos] = 0;
}

static void to_short_name(const char* name, uint8_t out[11]) {
    for (int i = 0; i < 11; i++) out[i] = ' ';

    int i = 0, pos = 0;
    while (name[i] != '\0' && name[i] != '.' && pos < 8) {
        char c = name[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[pos++] = (uint8_t)c;
    }
    while (name[i] != '\0' && name[i] != '.') i++;

    if (name[i] == '.') {
        i++;
        int epos = 0;
        while (name[i] != '\0' && epos < 3) {
            char c = name[i++];
            if (c >= 'a' && c <= 'z') c -= 32;
            out[8 + epos++] = (uint8_t)c;
        }
    }
}

static void reconstruct_lfn_part(FAT32_LFN_Entry* l, char* namebuf) {
    uint8_t seq = l->order & 0x3F;
    if (seq == 0) return;
    int base = (seq - 1) * 13;
    uint16_t chars[13];
    for (int i = 0; i < 5; i++) chars[i]      = l->name1[i];
    for (int i = 0; i < 6; i++) chars[5 + i]  = l->name2[i];
    for (int i = 0; i < 2; i++) chars[11 + i] = l->name3[i];
    for (int i = 0; i < 13; i++) {
        uint16_t c = chars[i];
        if (c == 0x0000 || c == 0xFFFF) break;
        namebuf[base + i] = (char)(c & 0xFF);
    }
}

int fat32_init(uint32_t partition_lba) {
    uint8_t boot_sector[512];
    read_sector(partition_lba, boot_sector);
    FAT32_BPB* bpb = (FAT32_BPB*)boot_sector;

    if (!(bpb->fs_type[0] == 'F' && bpb->fs_type[1] == 'A' && bpb->fs_type[2] == 'T' && bpb->fs_type[3] == '3')) {
        print("fat32_init: FAT32 signature not found, aborting\n");
        return 0;
    }

    g_partition_lba       = partition_lba;
    g_bytes_per_sector    = bpb->bytes_per_sector;
    g_sectors_per_cluster = bpb->sectors_per_cluster;
    g_fat_start_lba       = partition_lba + bpb->reserved_sectors;
    g_fat_count           = bpb->fat_count;
    g_sectors_per_fat32   = bpb->sectors_per_fat32;
    g_data_start_lba      = g_fat_start_lba + (uint32_t)bpb->fat_count * bpb->sectors_per_fat32;
    g_root_cluster        = bpb->root_cluster;
    g_cluster_size        = g_sectors_per_cluster * g_bytes_per_sector;

    uint32_t relative_data_start = g_data_start_lba - partition_lba;
    g_total_clusters = (bpb->total_sectors_32 > relative_data_start)
        ? (bpb->total_sectors_32 - relative_data_start) / g_sectors_per_cluster
        : 0;

    if (g_cluster_size > FAT32_MAX_CLUSTER_BYTES) {
        print("fat32_init: cluster size exceeds supported upper limit, aborting\n");
        g_mounted = 0;
        return 0;
    }

    g_fat_cache_sector = 0xFFFFFFFF;
    g_alloc_hint = 2;
    g_current_dir_cluster = g_root_cluster;
    g_mounted = 1;
    return 1;
}

uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    return g_data_start_lba + (cluster - 2) * g_sectors_per_cluster;
}

uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset  = cluster * 4;
    uint32_t fat_sector  = g_fat_start_lba + (fat_offset / g_bytes_per_sector);
    uint32_t entry_off   = fat_offset % g_bytes_per_sector;

    if (g_fat_cache_sector != fat_sector) {
        read_sector(fat_sector, g_fat_cache);
        g_fat_cache_sector = fat_sector;
    }

    uint32_t val = *(uint32_t*)(g_fat_cache + entry_off);
    return val & 0x0FFFFFFF;
}

static void fat32_set_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t sector_in_fat = fat_offset / g_bytes_per_sector;
    uint32_t entry_off     = fat_offset % g_bytes_per_sector;

    uint8_t sector_buf[512];
    uint32_t primary_sector = g_fat_start_lba + sector_in_fat;
    read_sector(primary_sector, sector_buf);

    uint32_t old_val = *(uint32_t*)(sector_buf + entry_off);
    uint32_t new_val = (old_val & 0xF0000000) | (value & 0x0FFFFFFF);

    *(uint32_t*)(sector_buf + entry_off) = new_val;

    for (uint32_t f = 0; f < g_fat_count; f++) {
        uint32_t lba = g_fat_start_lba + (f * g_sectors_per_fat32) + sector_in_fat;
        write_sector(lba, sector_buf);
    }

    if (g_fat_cache_sector == primary_sector) {
        *(uint32_t*)(g_fat_cache + entry_off) = new_val;
    }
}

static uint32_t fat32_alloc_cluster() {
    uint32_t limit = g_total_clusters + 2;
    for (uint32_t tries = 0; tries < g_total_clusters; tries++) {
        uint32_t c = g_alloc_hint;
        g_alloc_hint++;
        if (g_alloc_hint >= limit) g_alloc_hint = 2;

        if (fat32_get_next_cluster(c) == FAT32_FREE_CLUSTER) {
            fat32_set_fat_entry(c, 0x0FFFFFFF);
            return c;
        }
    }
    print("fat32_alloc_cluster: disk full!\n");
    return 0;
}

static void fat32_free_chain(uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        uint32_t next = fat32_get_next_cluster(cluster);
        fat32_set_fat_entry(cluster, FAT32_FREE_CLUSTER);
        cluster = next;
    }
}

void fat32_read_cluster(uint32_t cluster, uint8_t* buffer) {
    uint32_t lba = fat32_cluster_to_lba(cluster);
    for (uint32_t s = 0; s < g_sectors_per_cluster; s++) {
        read_sector(lba + s, buffer + (s * g_bytes_per_sector));
    }
}

static int fat32_write_cluster(uint32_t cluster, const uint8_t* buffer) {
    uint32_t lba = fat32_cluster_to_lba(cluster);
    for (uint32_t s = 0; s < g_sectors_per_cluster; s++) {
        if (!write_sector(lba + s, buffer + (s * g_bytes_per_sector))) return 0;
    }
    return 1;
}

static int fat32_walk_dir(uint32_t dir_cluster, int mode, const char* target_name, FAT32_DirectoryEntry* out_entry) {
    if (!g_mounted) return 0;

    char lfn_buf[256];
    for (int i = 0; i < 256; i++) lfn_buf[i] = 0;

    uint32_t cluster = dir_cluster;
    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        fat32_read_cluster(cluster, g_cluster_buf);

        for (uint32_t off = 0; off < g_cluster_size; off += 32) {
            uint8_t* raw = g_cluster_buf + off;
            uint8_t attr = raw[11];

            if (attr == FAT32_ATTR_LFN) {
                FAT32_LFN_Entry* l = (FAT32_LFN_Entry*)raw;
                if (l->order & 0x40) {
                    for (int i = 0; i < 256; i++) lfn_buf[i] = 0;
                }
                reconstruct_lfn_part(l, lfn_buf);
                continue;
            }

            FAT32_DirectoryEntry* e = (FAT32_DirectoryEntry*)raw;

            if (e->filename[0] == 0x00) return 0;
            if (e->filename[0] == 0xE5) { lfn_buf[0] = 0; continue; }
            if (e->attributes & FAT32_ATTR_VOLUME_ID) { lfn_buf[0] = 0; continue; }

            char short_name[13];
            format_short_name(e, short_name);
            const char* display_name = (lfn_buf[0] != 0) ? lfn_buf : short_name;

            if (mode == 0) {
                if (str_eq_ci(display_name, target_name) || str_eq_ci(short_name, target_name)) {
                    if (out_entry) *out_entry = *e;
                    return 1;
                }
            } else {
                print(display_name);
                if (e->attributes & FAT32_ATTR_DIRECTORY) print(" <DIR>");
                print("\n");
            }

            lfn_buf[0] = 0;
        }

        cluster = fat32_get_next_cluster(cluster);
    }
    return 0;
}

void fat32_list_directory(uint32_t dir_cluster) {
    fat32_walk_dir(dir_cluster, 1, 0, 0);
}

void fat32_list_root_directory() {
    fat32_list_directory(g_root_cluster);
}

void fat32_list_current_directory() {
    fat32_list_directory(g_current_dir_cluster);
}

int fat32_find_in_dir(uint32_t dir_cluster, const char* name, FAT32_DirectoryEntry* out_entry) {
    return fat32_walk_dir(dir_cluster, 0, name, out_entry);
}

int fat32_find(const char* name, FAT32_DirectoryEntry* out_entry) {
    return fat32_find_in_dir(g_current_dir_cluster, name, out_entry);
}

uint32_t fat32_load_file(const char* name, uint8_t* load_address) {
    FAT32_DirectoryEntry entry;
    if (!fat32_find(name, &entry)) return 0;

    uint32_t cluster  = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    uint32_t remaining = entry.file_size;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN && remaining > 0) {
        fat32_read_cluster(cluster, load_address);
        load_address += g_cluster_size;
        remaining = (remaining > g_cluster_size) ? (remaining - g_cluster_size) : 0;
        cluster = fat32_get_next_cluster(cluster);
    }
    return entry.file_size;
}

void fat32_cat_file(const char* name) {
    FAT32_DirectoryEntry entry;
    if (!fat32_find(name, &entry)) {
        print("\nError: File not found!\n");
        return;
    }

    uint32_t cluster  = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    uint32_t remaining = entry.file_size;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN && remaining > 0) {
        fat32_read_cluster(cluster, g_cluster_buf);
        uint32_t take = (remaining < g_cluster_size) ? remaining : g_cluster_size;
        for (uint32_t i = 0; i < take; i++) put_char(g_cluster_buf[i]);
        remaining -= take;
        cluster = fat32_get_next_cluster(cluster);
    }
    print("\n");
}

static int short_name_eq(const uint8_t* a, const uint8_t* b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

static int fat32_scan_dir_raw(uint32_t dir_cluster, int mode, const uint8_t short_name11[11],
                               uint32_t* out_lba, uint32_t* out_off, FAT32_DirectoryEntry* out_entry,
                               uint32_t* out_last_cluster) {
    uint32_t cluster = dir_cluster;
    uint32_t last_cluster = dir_cluster;

    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        last_cluster = cluster;
        uint32_t base_lba = fat32_cluster_to_lba(cluster);

        for (uint32_t s = 0; s < g_sectors_per_cluster; s++) {
            uint8_t sector_buf[512];
            read_sector(base_lba + s, sector_buf);

            for (uint32_t off = 0; off < g_bytes_per_sector; off += 32) {
                FAT32_DirectoryEntry* e = (FAT32_DirectoryEntry*)(sector_buf + off);

                if (mode == 1) {
                    if (e->filename[0] == 0x00 || e->filename[0] == 0xE5) {
                        if (out_lba) *out_lba = base_lba + s;
                        if (out_off) *out_off = off;
                        return 1;
                    }
                    continue;
                }

                if (e->filename[0] == 0x00) return 0;
                if (e->filename[0] == 0xE5) continue;
                if (e->attributes == FAT32_ATTR_LFN) continue;

                if (short_name_eq(e->filename, short_name11, 11)) {
                    if (out_lba) *out_lba = base_lba + s;
                    if (out_off) *out_off = off;
                    if (out_entry) *out_entry = *e;
                    return 1;
                }
            }
        }

        cluster = fat32_get_next_cluster(cluster);
    }

    if (out_last_cluster) *out_last_cluster = last_cluster;
    return 0;
}

int fat32_write_file(const char* name, const uint8_t* data, uint32_t size) {
    if (!g_mounted) return 0;

    uint8_t short_name[11];
    to_short_name(name, short_name);

    uint32_t dir_cluster = g_current_dir_cluster;

    uint32_t existing_lba, existing_off, last_cluster_in_dir;
    FAT32_DirectoryEntry existing;
    int found = fat32_scan_dir_raw(dir_cluster, 0, short_name, &existing_lba, &existing_off, &existing, &last_cluster_in_dir);

    if (found) {
        uint32_t old_cluster = ((uint32_t)existing.first_cluster_high << 16) | existing.first_cluster_low;
        if (old_cluster >= 2) fat32_free_chain(old_cluster);
    }

    uint32_t clusters_needed = (size == 0) ? 0 : ((size + g_cluster_size - 1) / g_cluster_size);
    uint32_t first_cluster = 0;
    uint32_t prev_cluster = 0;

    for (uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t c = fat32_alloc_cluster();
        if (c == 0) {
            if (first_cluster) fat32_free_chain(first_cluster);
            return 0;
        }
        if (first_cluster == 0) first_cluster = c;
        if (prev_cluster != 0) fat32_set_fat_entry(prev_cluster, c);
        prev_cluster = c;
    }
    if (prev_cluster != 0) fat32_set_fat_entry(prev_cluster, 0x0FFFFFFF);

    uint32_t cluster = first_cluster;
    uint32_t remaining = size;
    while (cluster >= 2 && remaining > 0) {
        for (uint32_t i = 0; i < g_cluster_size; i++) g_cluster_buf[i] = 0;
        uint32_t take = (remaining < g_cluster_size) ? remaining : g_cluster_size;
        for (uint32_t i = 0; i < take; i++) g_cluster_buf[i] = data[i];

        fat32_write_cluster(cluster, g_cluster_buf);

        data += take;
        remaining -= take;
        cluster = fat32_get_next_cluster(cluster);
    }

    FAT32_DirectoryEntry entry;
    uint8_t* raw = (uint8_t*)&entry;
    for (uint32_t i = 0; i < sizeof(FAT32_DirectoryEntry); i++) raw[i] = 0;
    for (int i = 0; i < 8; i++) entry.filename[i]  = short_name[i];
    for (int i = 0; i < 3; i++) entry.extension[i] = short_name[8 + i];
    entry.attributes        = FAT32_ATTR_ARCHIVE;
    entry.first_cluster_high = (uint16_t)(first_cluster >> 16);
    entry.first_cluster_low  = (uint16_t)(first_cluster & 0xFFFF);
    entry.file_size          = size;

    uint32_t target_lba, target_off;
    if (found) {
        target_lba = existing_lba;
        target_off = existing_off;
    } else {
        if (!fat32_scan_dir_raw(dir_cluster, 1, 0, &target_lba, &target_off, 0, &last_cluster_in_dir)) {
            uint32_t new_dir_cluster = fat32_alloc_cluster();
            if (new_dir_cluster == 0) {
                if (first_cluster) fat32_free_chain(first_cluster);
                return 0;
            }
            fat32_set_fat_entry(last_cluster_in_dir, new_dir_cluster);
            fat32_set_fat_entry(new_dir_cluster, 0x0FFFFFFF);

            for (uint32_t i = 0; i < g_cluster_size; i++) g_cluster_buf[i] = 0;
            fat32_write_cluster(new_dir_cluster, g_cluster_buf);

            target_lba = fat32_cluster_to_lba(new_dir_cluster);
            target_off = 0;
        }
    }

    uint8_t sector_buf[512];
    read_sector(target_lba, sector_buf);
    uint8_t* dst = sector_buf + target_off;
    for (uint32_t i = 0; i < sizeof(FAT32_DirectoryEntry); i++) dst[i] = raw[i];
    write_sector(target_lba, sector_buf);

    return 1;
}

int fat32_make_directory(const char* name) {
    if (!g_mounted) return 0;

    uint32_t new_cluster = fat32_alloc_cluster();
    if (new_cluster == 0) return 0;
    fat32_set_fat_entry(new_cluster, 0x0FFFFFFF);

    for (uint32_t i = 0; i < g_cluster_size; i++) g_cluster_buf[i] = 0;

    FAT32_DirectoryEntry* dot = (FAT32_DirectoryEntry*)g_cluster_buf;
    dot->filename[0] = '.';
    for (int i = 1; i < 8; i++) dot->filename[i] = ' ';
    for (int i = 0; i < 3; i++) dot->extension[i] = ' ';
    dot->attributes = FAT32_ATTR_DIRECTORY;
    dot->first_cluster_high = (uint16_t)(new_cluster >> 16);
    dot->first_cluster_low  = (uint16_t)(new_cluster & 0xFFFF);
    dot->file_size = 0;

    FAT32_DirectoryEntry* dotdot = (FAT32_DirectoryEntry*)(g_cluster_buf + 32);
    dotdot->filename[0] = '.'; dotdot->filename[1] = '.';
    for (int i = 2; i < 8; i++) dotdot->filename[i] = ' ';
    for (int i = 0; i < 3; i++) dotdot->extension[i] = ' ';
    dotdot->attributes = FAT32_ATTR_DIRECTORY;
    uint32_t parent = (g_current_dir_cluster == g_root_cluster) ? 0 : g_current_dir_cluster;
    dotdot->first_cluster_high = (uint16_t)(parent >> 16);
    dotdot->first_cluster_low  = (uint16_t)(parent & 0xFFFF);
    dotdot->file_size = 0;

    fat32_write_cluster(new_cluster, g_cluster_buf);

    uint8_t short_name[11];
    to_short_name(name, short_name);

    uint32_t target_lba, target_off, last_cluster_in_dir;
    if (!fat32_scan_dir_raw(g_current_dir_cluster, 1, 0, &target_lba, &target_off, 0, &last_cluster_in_dir)) {
        uint32_t extra = fat32_alloc_cluster();
        if (extra == 0) { fat32_free_chain(new_cluster); return 0; }
        fat32_set_fat_entry(last_cluster_in_dir, extra);
        fat32_set_fat_entry(extra, 0x0FFFFFFF);

        for (uint32_t i = 0; i < g_cluster_size; i++) g_cluster_buf[i] = 0;
        fat32_write_cluster(extra, g_cluster_buf);

        target_lba = fat32_cluster_to_lba(extra);
        target_off = 0;
    }

    FAT32_DirectoryEntry entry;
    uint8_t* raw = (uint8_t*)&entry;
    for (uint32_t i = 0; i < sizeof(FAT32_DirectoryEntry); i++) raw[i] = 0;
    for (int i = 0; i < 8; i++) entry.filename[i]  = short_name[i];
    for (int i = 0; i < 3; i++) entry.extension[i] = short_name[8 + i];
    entry.attributes         = FAT32_ATTR_DIRECTORY;
    entry.first_cluster_high = (uint16_t)(new_cluster >> 16);
    entry.first_cluster_low  = (uint16_t)(new_cluster & 0xFFFF);
    entry.file_size          = 0;

    uint8_t sector_buf[512];
    read_sector(target_lba, sector_buf);
    uint8_t* dst = sector_buf + target_off;
    for (uint32_t i = 0; i < sizeof(FAT32_DirectoryEntry); i++) dst[i] = raw[i];
    write_sector(target_lba, sector_buf);

    return 1;
}

int fat32_change_directory(const char* name) {
    if (!g_mounted) return 0;

    if (compare_string_local(name, ".")) return 1;

    if (compare_string_local(name, "..")) {
        FAT32_DirectoryEntry dotdot;
        if (!fat32_find_in_dir(g_current_dir_cluster, "..", &dotdot)) return 0;
        uint32_t parent = ((uint32_t)dotdot.first_cluster_high << 16) | dotdot.first_cluster_low;
        g_current_dir_cluster = (parent == 0) ? g_root_cluster : parent;
        return 1;
    }

    FAT32_DirectoryEntry entry;
    if (!fat32_find_in_dir(g_current_dir_cluster, name, &entry)) {
        print("cd: directory not found\n");
        return 0;
    }
    if (!(entry.attributes & FAT32_ATTR_DIRECTORY)) {
        print("cd: not a directory\n");
        return 0;
    }

    uint32_t cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    g_current_dir_cluster = (cluster == 0) ? g_root_cluster : cluster;
    return 1;
}

uint32_t fat32_get_current_dir_cluster() {
    return g_current_dir_cluster;
}
