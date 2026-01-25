#include <kernel.h>
#include <graphics.h>
#include <fonts/tvga8800cs__8x16.h>

#define VESA_W 1024
#define VESA_H 768

unsigned int current_color = 0xFFFFFFFF;
int cursor_x = 0; int cursor_y = 0;
unsigned int* vesa = (unsigned int*)0xFD000000;

void draw_pixel(int x, int y, unsigned int color) {
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
        for(int i=0; i<16; i++)
            for(int j=0; j<8; j++)
                draw_pixel((cursor_x * 8) + j, (cursor_y * 16) + i, 0x00);
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
    print("AiraOS v0.2 \n");
    print("Mem: ");
    print_int((mem_limit - free_mem_addr) / 1024);
    print("\n");
}
