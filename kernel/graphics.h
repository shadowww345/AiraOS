#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <sound.h>
extern unsigned int current_color;
extern int cursor_x;
extern int cursor_y;

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} bmp_header_t;

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_m;
    int32_t  y_pixels_per_m;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_info_header_t;
#pragma pack(pop)


void draw_pixel(int x, int y, unsigned int color);
void draw_char(char c, int x, int y, unsigned int color);
void update_cursor(int x, int y);
void put_char(char c);
void print(const char* str);
void print_int(int n);
void clear_screen();
void clear_color(unsigned int color);
void set_background(unsigned int color);
void draw_status_bar();
void blit_buffer(int x, int y, int w, int h, const unsigned int* src);
void draw_bmp(int start_x, int start_y, int dest_w, int dest_h, const char* filename);

#endif
