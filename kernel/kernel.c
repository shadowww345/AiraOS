
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char val);

char file_system_buffer[4096]; 
int file_size = 0;

int cursor_x = 0; int cursor_y = 0;
volatile char *video = (char*)0xB8000;
unsigned char current_color = 0x0F;
void play_sound(unsigned int nFrequence) {
    unsigned int Div;
    unsigned char tmp;

    Div = 1193180 / nFrequence;
    
    outb(0x43, 0xB6);
    outb(0x42, (unsigned char) (Div) );
    outb(0x42, (unsigned char) (Div >> 8));

    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void stop_sound() {
    unsigned char tmp = (inb(0x61) & 0xFC);
    outb(0x61, tmp);
}

void beep() {
    play_sound(1200);
    for(volatile int i = 0; i < 10000000; i++); 
    stop_sound();
}

unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '(', ')', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '#', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '"', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '{', '}', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int compare_string(char* s1, char* s2) {
    int i = 0;
    while (s1[i] == s2[i]) {
        if (s1[i] == '\0') return 1;
        i++;
    }
    return 0;
}

int starts_with(char* str, char* prefix) {
    int i = 0;
    while (prefix[i] != '\0') {
        if (str[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

void update_cursor(int x, int y) {
    unsigned short pos = y * 80 + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void put_char(char c) {
    if (c == '\b') { 
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = 79;
        }
        int pos = (cursor_y * 80 + cursor_x) * 2;
        video[pos] = ' ';
        video[pos+1] = current_color;
    } else if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        int pos = (cursor_y * 80 + cursor_x) * 2;
        video[pos] = c;
        video[pos+1] = current_color;
        cursor_x++;
    }
    
    if (cursor_x >= 80) { cursor_x = 0; cursor_y++; }
    update_cursor(cursor_x, cursor_y);
}

void print(const char* str) { for (int i = 0; str[i] != '\0'; i++) put_char(str[i]); }

void clear_screen() {
    for (int i = 0; i < 80 * 25 * 2; i += 2) { video[i] = ' '; video[i+1] = current_color; }
    cursor_x = 0; cursor_y = 0; update_cursor(0, 0);
}
//I’m still developing this, it’s like my own little language.
void run_interper() {
    if (file_size == 0) { print("No code founded!\n"); return; }
    if (starts_with(file_system_buffer, "print(\"")) {
        int start = 7; int end = 0;
        for (int i = start; i < file_size; i++) {
            if (file_system_buffer[i] == '\"') { end = i; break; }
        }
        
        if (end > start) {
           
            for (int i = start; i < end; i++) put_char(file_system_buffer[i]);
            print("\n");
        }
         else print("Syntax Error\n");
    } else print("Undefined function\n");
}

void run_nano() {
    clear_screen();
    print("---NANO | ESC: Save\n");
    file_size = 0;
    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                if (scancode == 0x01) break; 
                char c = keyboard_map[scancode];
                
                if (c == '\b') {
                    if (file_size > 0) {
                        file_size--;
                        put_char('\b');
                    }
                } else if (c != 0 && file_size < 4095) {
                    file_system_buffer[file_size++] = c;
                    put_char(c);
                }
            }
        }
    }
    file_system_buffer[file_size] = '\0';
    clear_screen();
    print("Saved\n> ");
}

void kernel_main() {
    clear_screen();
    print("Welcome To AiraOS!\n> ");
    char cmd[64]; int idx = 0;

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if (c == '\n') {
                    cmd[idx] = '\0'; put_char('\n');
                    
                    if (compare_string(cmd, "nano")) run_nano();
                    else if (compare_string(cmd, "compile")) run_interper();
                    else if (compare_string(cmd, "clear")) clear_screen();
                    else if (compare_string(cmd, "color a")) current_color = 0x0A;
                    else if (compare_string(cmd, "color b")) current_color = 0x0B;
                    else if (compare_string(cmd, "color c")) current_color = 0x0C;
                    else if (compare_string(cmd, "color 7")) current_color = 0x07;
                    else if (compare_string(cmd, "beep")) {beep();}
                    else if (idx > 0) print("Undefined Command\n");
                    
                    idx = 0; print("> ");
                } else if (c == '\b' && idx > 0) {
                    idx--; put_char('\b');
                } else if (c != 0 && idx < 63) {
                    cmd[idx++] = c;
                    put_char(c);
                }
            }
        }
    }
}
