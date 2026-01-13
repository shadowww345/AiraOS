
/*
 * [ AiraOS v2.0 - GUI Note ]
 * * I tried to implement a Graphical User Interface (GUI), but it is 
 * quite complex and time-consuming at this stage. 
 *
 * For now, I am focusing on the kernel core and backend logic. 
 * If anyone wants to develop a GUI for AiraOS, feel free to contribute!
 * Maybe ı make little GUI
 */

typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

#define SB16_PORT_BASE 0x220
#define SB16_RESET     (SB16_PORT_BASE + 0x6)
#define SB16_READ      (SB16_PORT_BASE + 0xA)
#define SB16_WRITE     (SB16_PORT_BASE + 0xC)
#define SB16_STATUS    (SB16_PORT_BASE + 0xE)
void setup_dma(uint32_t buffer_addr, uint32_t length);
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char val);

typedef enum { TYPE_INT, TYPE_STRING, TYPE_FLOAT } var_type;


int reset_sb16() {
    outb(SB16_RESET, 1);
    for(int i = 0; i < 1000; i++) { asm("nop"); }
    outb(SB16_RESET, 0);
    for(int timeout = 0; timeout < 10000; timeout++) {
        if (inb(SB16_STATUS) & 0x80) {
            if (inb(SB16_READ) == 0xAA) {
                return 1;
            }
        }
    }
    return 0;
}


void sb16_write_dsp(uint8_t data) {
    while (inb(SB16_WRITE) & 0x80);
    outb(SB16_WRITE, data);
}

uint8_t sb16_read_dsp() {
    while (!(inb(SB16_STATUS) & 0x80));
    return inb(SB16_READ);
}

void setup_dma(uint32_t buffer_addr, uint32_t length) {
    outb(0xD4, 5 | 4);
    outb(0xD8, 0);
    outb(0xD6, 0x49);
    uint8_t page = (buffer_addr >> 16) & 0xFF;
    uint16_t offset = (uint16_t)((buffer_addr >> 1) & 0xFFFF);

    outb(0x8B, page);
    outb(0xC4, (uint8_t)(offset & 0xFF));
    outb(0xC4, (uint8_t)((offset >> 8) & 0xFF));
    uint16_t count = (uint16_t)((length >> 1) - 1);
    outb(0xC6, (uint8_t)(count & 0xFF));
    outb(0xC6, (uint8_t)((count >> 8) & 0xFF));

    outb(0xD4, 5);
}
uint8_t random_buffer[65536];
void play_noise() {
    for (int i = 0; i < 65536; i++) {
        random_buffer[i] = (unsigned char)((i * i) ^ (i >> 4) ^ (i << 3));
    }
    setup_dma((uint32_t)random_buffer, 65536);

    sb16_write_dsp(0xD1);
    sb16_write_dsp(0xC6); 
    sb16_write_dsp(0x00);
    sb16_write_dsp(0xFF); 
    sb16_write_dsp(0xFF);
}

struct Variable {
    char name[16];
    var_type type;
    int ival;
    float fval;    
    char sval[64];
    int active;
};

struct Variable var_table[50];


char file_system_buffer[4096]; 
int file_size = 0;

int cursor_x = 0; int cursor_y = 0;
volatile char *video = (char*)0xB8000;

unsigned char current_color = 0x0F;
uint32_t free_mem_addr = 0x100000; 
uint32_t mem_limit = 0x200000;



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

void draw_pixel(int x, int y, unsigned char color) {
    unsigned char* vga = (unsigned char*)0xA0000;
    vga[y * 320 + x] = color;
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
void sleep(int duration) {
    for(volatile int i = 0; i < duration * 1000000; i++) {

    }
}

void beep_with_duration(unsigned int freq, int duration) {
    if (freq > 0) {
        play_sound(freq);
        sleep(duration);
        stop_sound();
    }
}
void beep_freq(unsigned int freq) {
    if (freq == 0) {
        stop_sound();
    } else {
        play_sound(freq); 
    }
}
int find_var(char* name) {
    for (int i = 0; i < 50; i++) {
        if (var_table[i].active && compare_string(var_table[i].name, name)) {
            return i; 
        }
    }
    return -1;
}
void clear_screen() {
    for (int i = 0; i < 80 * 25 * 2; i += 2) { video[i] = ' '; video[i+1] = current_color; }
    cursor_x = 0; cursor_y = 0; update_cursor(0, 0);
}
int loop_count = 0;
int loop_start_index = 0;
void get_string(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if (c == '\n') break;
                if (c == '\b' && i > 0) {
                    i--; put_char('\b');
                } else if (c != 0) {
                    buffer[i++] = c;
                    put_char(c);
                }
            }
        }
    }
    buffer[i] = '\0';
}
struct File {
    char name[32];
    char content[1024];
    int size;
    int active;
};

struct File files[10];
void process_math(int op, int *i) {
    char vname[16]; int v_ptr = 0;
    while (file_system_buffer[(*i)] != ',' && file_system_buffer[(*i)] != ' ' && v_ptr < 15) {
        vname[v_ptr++] = file_system_buffer[(*i)++];
    }
    vname[v_ptr] = '\0';
    while (file_system_buffer[(*i)] == ',' || file_system_buffer[(*i)] == ' ') (*i)++;
    int val = 0;
    while (file_system_buffer[(*i)] >= '0' && file_system_buffer[(*i)] <= '9') {
        val = val * 10 + (file_system_buffer[(*i)] - '0');
        (*i)++;
    }
    int idx = find_var(vname);
    if (idx != -1 && var_table[idx].type == TYPE_INT) {
        if (op == 1) var_table[idx].ival += val;
        else if (op == 2) var_table[idx].ival -= val;
        else if (op == 3) var_table[idx].ival *= val;
        else if (op == 4 && val != 0) var_table[idx].ival /= val;
    }
}
//For lua. Under Development
void* malloc(int size) {
    if (size % 4 != 0) size += (4 - (size % 4));

    if (free_mem_addr + size > mem_limit) {
        print("Out of memory\n");
        beep_freq(60);
        return 0;
    }
    void* res = (void*)free_mem_addr;
    free_mem_addr += size;
    char* p = (char*)res;
    for(int i=0; i<size; i++) p[i] = 0;

    return res;
}
void free(void* ptr) {}
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

void draw_status_bar() {
    print("AiraOS v0.2 \n");
    print("Mem: ");
    print_int((mem_limit - free_mem_addr) / 1024);
    print("\n");
}
//I’m still developing this, it’s like my own little language.
void run_interper() {
    if (file_size == 0) { print("No code found!\n"); return; }
    int   vars_int[10];
    float vars_float[10];
    char  vars_string[10][64];
    int i = 0;
    while (i < file_size) {
  
    if (file_system_buffer[i] == ' ' || file_system_buffer[i] == '\n' || file_system_buffer[i] == '\r') {
            i++;
            continue;
        }
    else if (starts_with(&file_system_buffer[i], "print(")) {
    i += 6;
    if (file_system_buffer[i] == '\"') {
        i++;
        while (i < file_size && file_system_buffer[i] != '\"') {
            put_char(file_system_buffer[i++]);
        }
        i++;
    } else {
        char vname[16]; int v_p = 0;
        while(file_system_buffer[i] != ')' && file_system_buffer[i] != ' ' && v_p < 15) {
            vname[v_p++] = file_system_buffer[i++];
        }
        vname[v_p] = '\0';
        int idx = find_var(vname);
        if (idx != -1) {
            if (var_table[idx].type == TYPE_STRING) {
                print(var_table[idx].sval);
            } else if (var_table[idx].type == TYPE_INT) {
                print_int(var_table[idx].ival);
            }
        } else {
            print("Undefined Variable");
        }
    }
    if (file_system_buffer[i] == ')') i++;
    print("\n");
}
        else if (starts_with(&file_system_buffer[i], "color ")) {
            i += 6; 
            char color_code = file_system_buffer[i];
            if (color_code == 'a') current_color = 0x0A;
            else if (color_code == 'b') current_color = 0x0B;
            else if (color_code == 'c') current_color = 0x0C;
            else if (color_code == '7') current_color = 0x07;
            i++;
        }
      
        else if (starts_with(&file_system_buffer[i], "clear")) {
            clear_screen();
            i += 5;
        }
        else if (starts_with(&file_system_buffer[i], "sleep(")) {
            i += 6;
            int val = 0;
            
            while(file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
                val = val * 10 + (file_system_buffer[i] - '0');
                i++;
            }
            sleep(val);
        if (file_system_buffer[i] == ')') i++; 
        }
else if (starts_with(&file_system_buffer[i], "beep(")) {
    i += 5;
    
    int freq = 0;
    int duration = 0;

    if (file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
        while(file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
            freq = freq * 10 + (file_system_buffer[i] - '0');
            i++;
        }
    } else {
        char vname[16]; int v_p = 0;
        while(file_system_buffer[i] != ',' && file_system_buffer[i] != ')' && file_system_buffer[i] != ' ')
            vname[v_p++] = file_system_buffer[i++];
        vname[v_p] = '\0';
        int idx = find_var(vname);
        if(idx != -1) freq = var_table[idx].ival;
    }
    while(file_system_buffer[i] == ' ' || file_system_buffer[i] == ',') i++;
    if (file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
        while(file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
            duration = duration * 10 + (file_system_buffer[i] - '0');
            i++;
        }
    } else if (file_system_buffer[i] != ')') {
        char vname[16]; int v_p = 0;
        while(file_system_buffer[i] != ')' && file_system_buffer[i] != ' ')
            vname[v_p++] = file_system_buffer[i++];
        vname[v_p] = '\0';
        int idx = find_var(vname);
        if(idx != -1) duration = var_table[idx].ival;
    }

   
    if (duration > 0) {
        beep_with_duration(freq, duration);
    } else {
        beep_freq(freq);
        for(volatile int i = 0; i < 10000000; i++); 
        beep_freq(0);
    }

    if (file_system_buffer[i] == ')') i++;
}
      
else if (starts_with(&file_system_buffer[i], "set ")) {
    i += 4;
    char vname[16];
    int v_ptr = 0;
    while (file_system_buffer[i] != ' ' && file_system_buffer[i] != '=' && v_ptr < 15) {
        vname[v_ptr++] = file_system_buffer[i++];
    }
    vname[v_ptr] = '\0';
    while (file_system_buffer[i] == ' ' || file_system_buffer[i] == '=') i++;
    int v_idx = find_var(vname);
    if (v_idx == -1) {
        for(int s=0; s<50; s++) {
            if(!var_table[s].active) { v_idx = s; break; }
        }
    }

    if (v_idx != -1) {
        var_table[v_idx].active = 1;
        for(int n=0; vname[n] != '\0'; n++) var_table[v_idx].name[n] = vname[n];

        if (file_system_buffer[i] == '\"') {
            i++;
            var_table[v_idx].type = TYPE_STRING;
            int s_ptr = 0;
            while (file_system_buffer[i] != '\"' && s_ptr < 63) {
                var_table[v_idx].sval[s_ptr++] = file_system_buffer[i++];
            }
            var_table[v_idx].sval[s_ptr] = '\0';
            i++;
        } else {
            var_table[v_idx].type = TYPE_INT;
            int val = 0;
            while (file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
                val = val * 10 + (file_system_buffer[i] - '0');
                i++;
            }
            var_table[v_idx].ival = val;
        }
    }
}
else if (starts_with(&file_system_buffer[i], "add ")) {
    i += 4;
    process_math(1, &i);
}
else if (starts_with(&file_system_buffer[i], "sub ")) {
    i += 4;
    process_math(2, &i);
}
else if (starts_with(&file_system_buffer[i], "mul ")) {
    i += 4;
    process_math(3, &i);
}

else if (starts_with(&file_system_buffer[i], "div ")) {
    i += 4;
    process_math(4, &i);
}
else if (starts_with(&file_system_buffer[i], "loop ")) {
    i += 5;
    int count = 0;
    

    if (file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
        while(file_system_buffer[i] >= '0' && file_system_buffer[i] <= '9') {
            count = count * 10 + (file_system_buffer[i] - '0');
            i++;
        }
    }

    loop_count = count;
    loop_start_index = i;
}

else if (starts_with(&file_system_buffer[i], "endloop")) {
    if (loop_count > 1) {
        loop_count--;
        i = loop_start_index;
    } else {
        i += 7;
    }
}
    else {
            
            i++;
            
        }
        
    }
}

void run_nano() {
    clear_screen();
    print("--- NANO | ESC to SAVE ---\n");
    
    char temp_buffer[1024];
    int temp_size = 0;

    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                if (scancode == 0x01) break;
                
                char c = keyboard_map[scancode];
                if (c == '\b' && temp_size > 0) {
                    temp_size--; put_char('\b');
                } else if (c != 0 && temp_size < 1023) {
                    temp_buffer[temp_size++] = c;
                    put_char(c);
                }
            }
        }
    }
    temp_buffer[temp_size] = '\0';

    print("\nSave as (filename.aira): ");
    char fname[32];
    get_string(fname, 32);

    if (fname[0] != '\0') {
        for(int i=0; i<10; i++) {
            if(!files[i].active) {
                for(int n=0; fname[n] != '\0'; n++) files[i].name[n] = fname[n];
                for(int m=0; m<=temp_size; m++) files[i].content[m] = temp_buffer[m];
                files[i].size = temp_size;
                files[i].active = 1;
                print("\nFile saved successfully!");
                break;
            }
        }
    }
    sleep(100);
    clear_screen();
    draw_status_bar();
}
void infinite_doom() {
    int buffer[1024];
    infinite_doom();
}
void kernel_main() {
    clear_screen();
    beep_with_duration(750,30);
    print("BOOTING:AiraOS v2.0 \n");
    if (reset_sb16()) {
        print("[OK]Sound Device Successfully initilazed\n");
        //if you love your ears dont delete '//'
        //play_noise();

    } else {
        print("[ERROR]Sound Device initilaze failed\n");
    }
    for(volatile int i=0; i<1000000000; i++);
    print("BOOTING:İnitizaled ALL \n");
    print("BOOTING:Successfully Booted \n");
    print("BOOTING:ALL OK \n");
    for(volatile int i=0; i<1000000000; i++);  
    //These words? Just here to look fancy, literally means nothing.
    clear_screen();
    draw_status_bar();
    print("Welcome To AiraOS!\n> ");
    char cmd[64]; int idx = 0;
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
    outb(0x60, 0xED); 
    outb(0x60, 0x07);
    outb(0x0A, 0x01);
    while(1) {
        if (inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if (!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                if (c == '\n') {
                    cmd[idx] = '\0'; put_char('\n');
                    
                    if (compare_string(cmd, "nano")) run_nano();
                    else if (starts_with(cmd, "compile ")) {
                        char* target_file = &cmd[8];
                        int found = 0;
                        for(int i=0; i<10; i++) {
                        if(files[i].active && compare_string(files[i].name, target_file)) {
                        for(int j=0; j<files[i].size; j++) file_system_buffer[j] = files[i].content[j];
                        file_size = files[i].size;
                        run_interper();
                        found = 1;
                        break;
                    }
                 }
                    if(!found) print("File not found!\n");
            }
                    else if (compare_string(cmd, "clear")) clear_screen();
                    else if (compare_string(cmd, "color a")) current_color = 0x0A;
                    else if (compare_string(cmd, "color b")) current_color = 0x0B;
                    else if (compare_string(cmd, "color c")) current_color = 0x0C;
                    else if (compare_string(cmd, "color 7")) current_color = 0x07;
                    else if (compare_string(cmd, "beep")) {beep();}
                    else if (compare_string(cmd, "safemode")) { // Very safe mode for cpu : )
                        beep_freq(70);
                        print("GORDON GET AWAY FROM THE MACHINE \n"); // I forgot what he says.
                        for(volatile int i=0; i<10000000; i++);
                        print("Shutting down \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("Attempting shutdown \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("Is not is not shutting down i- is not \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("AAAAAAAAAA \n");
                        for(volatile int i=0; i<100000000; i++);
                        volatile int a = 5;
                        volatile int b = 0;
                        int c = a / b;
                        void (*crash_ptr)() = (void (*)())0x12345678;
                        crash_ptr();
                        infinite_doom();
                        unsigned char *ptr = (unsigned char *)0x0;
                        while(1) {
                            *ptr = 0;
                            ptr++;
                        }
                        
                    }
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






