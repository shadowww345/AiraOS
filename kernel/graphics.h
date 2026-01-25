#ifndef GRAPHICS_H
#define GRAPHICS_H


extern unsigned int current_color;
extern int cursor_x;
extern int cursor_y;

void draw_pixel(int x, int y, unsigned int color);
void draw_char(char c, int x, int y, unsigned int color);
void update_cursor(int x, int y);
void put_char(char c);
void print(const char* str);
void print_int(int n);
void clear_screen();
void clear_color();
void set_background(unsigned int color);
void draw_status_bar();


#endif