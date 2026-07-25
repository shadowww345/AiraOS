#include <kernel.h>
#include <graphics.h>
#include <fonts/tvga8800cs__8x16.h>
#include <task.h>
#include <fat32.h>
#include <sound.h>
#define VESA_W 1024
#define VESA_H 768

unsigned int current_color = 0xFFFFFFFF;
int cursor_x = 0; int cursor_y = 0;
unsigned int* vesa = (unsigned int*)0xFD000000;

void draw_bmp(int start_x, int start_y, int dest_w, int dest_h, const char* filename) {
    uint8_t* file_buf = (uint8_t*)0x400000;
    
    uint32_t bytes_read = fat32_load_file((char*)filename, file_buf);
    if (bytes_read == 0) {
        print("file not found or empty\n");
        return;
    }

    bmp_header_t* header = (bmp_header_t*)file_buf;
    if (header->type != 0x4D42) {
        print("invalid bmp signature\n");
        return;
    }

    bmp_info_header_t* info = (bmp_info_header_t*)(file_buf + sizeof(bmp_header_t));
    if (info->bits_per_pixel != 24 || info->compression != 0) {
        print("only 24-bit uncompressed bmps supported\n");
        return;
    }

    int src_w = info->width;
    int src_h = info->height;
    int is_bottom_up = 1;

    if (src_h < 0) {
        src_h = -src_h;
        is_bottom_up = 0;
    }

    int row_stride = (src_w * 3 + 3) & ~3;
    uint8_t* pixel_data = file_buf + header->offset;
    for (int dy = 0; dy < dest_h; dy++) {
        int src_y = (dy * src_h) / dest_h;
        int row = is_bottom_up ? (src_h - 1 - src_y) : src_y;
        uint8_t* row_data = pixel_data + row * row_stride;

        for (int dx = 0; dx < dest_w; dx++) {
            int src_x = (dx * src_w) / dest_w;

            uint8_t b = row_data[src_x * 3 + 0];
            uint8_t g = row_data[src_x * 3 + 1];
            uint8_t r = row_data[src_x * 3 + 2];

            unsigned int color = (r << 16) | (g << 8) | b;
            draw_pixel(start_x + dx, start_y + dy, color);
        }
    }
}

void draw_pixel(int x, int y, unsigned int color) {
    yield();
    if (x >= 0 && x < VESA_W && y >= 0 && y < VESA_H) {
        vesa[y * VESA_W + x] = color;
    }
}

void draw_char(char c, int x, int y, unsigned int color) {
    unsigned char *glyph = &font_data[(unsigned char)c * 16]; 
    for (int i = 0; i < 16; i++) {
        unsigned char row = glyph[i];
        for (int j = 0; j < 8; j++) {
            if (row & (0x80 >> j)) {
                draw_pixel(x + j, y + i, color);
            }
        }
    }
}

void update_cursor(int x, int y) {
    unsigned short pos = y * 80 + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void put_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
        
        for(int i = 0; i < 16; i++) {
            for(int j = 0; j < 8; j++) {
                draw_pixel((cursor_x * 8) + j, (cursor_y * 16) + i, 0x00000000); 
            }
        }
    } else {
        draw_char(c, cursor_x * 8, cursor_y * 16, current_color);
        cursor_x++;
    }

    if (cursor_x >= 128) {
        cursor_x = 0;
        cursor_y++;
    }
}

void print(const char* str) { for (int i = 0; str[i] != '\0'; i++) put_char(str[i]); }

void print_int(int n) {
    if (n == 0) {
        put_char('0');
        return;
    }
    if (n < 0) {
        put_char('-');
        n = -n;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) {
        put_char(buf[i]);
    }
}

void clear_color(unsigned int color) {
    for (int i = 0; i < VESA_W * VESA_H; i++) {
        vesa[i] = color;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void clear_screen() {
    for (int i = 0; i < VESA_W * VESA_H; i++) {
        vesa[i] = 0; 
    }
    cursor_x = 0;
    cursor_y = 0;
}

void set_background(unsigned int color) {
    for (int i = 0; i < VESA_W * VESA_H; i++) {
        vesa[i] = color;
    }
}

void draw_status_bar() {
    print("AiraOS v3.0 \n");
    print("Mem: ");
    print_int((mem_limit - free_mem_addr) / 1024);
    print("\n");
}

void blit_buffer(int x, int y, int w, int h, const unsigned int* src) {
    for (int row = 0; row < h; row++) {
        int py = y + row;
        if (py < 0 || py >= VESA_H) continue;
        for (int col = 0; col < w; col++) {
            int px = x + col;
            if (px < 0 || px >= VESA_W) continue;
            vesa[py * VESA_W + px] = src[row * w + col];
        }
    }
}