#include <kernel.h>
#include <graphics.h>
#include <aira_lang.h>
#include <sound.h>
#include <fat32.h>
#include <idt.h>
#include <asm.h>
#include <task.h>

unsigned int bgcolor = 0xFF0033;
const char *username="Live-Session-User";
int file_size = 0; 
char file_system_buffer[4096]; 
struct File files[10];
uint32_t free_mem_addr = 0x100000; 
uint32_t mem_limit = 0x1000000;
void reboot() {
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    while(1) {  }
}

const char* aira_os_panic= R"(
       ,___, 
       (x,x)
       /)_)
   ---""--""---
   A I R A  O S
)";

const char* aira_os= R"(
       ,___,
       (0,0) 
       /)_) 
   ---""--""--- 
   A I R A  O S 
)";
void panic(const char* messg) {
    __asm__("cli");
    clear_screen();
    set_background(0xFF003399);
    current_color = 0xFF000000;
    print(aira_os_panic);
    print("KERNEL PANIC: \n");
    print(messg);
    print("\n Please reboot your computer manually. \n");
    for(;;) {
        __asm__("hlt");
    }
}


void timer_test_handler() {
    beep_freq(600);
}

unsigned char keyboard_map_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '~', '`', 0,
    '|','Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '"', 0,  '*', 0,  ' ',
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    '{', '}', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};


unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '(', ')', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '#', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '"', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '{', '}', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static int g_shift_pressed = 0;
char poll_keyboard() {
    if (!(inb(0x64) & 0x01)) return 0;
 
    unsigned char scancode = inb(0x60);
    unsigned char released = scancode & 0x80;
    unsigned char code     = scancode & 0x7F;
 
    if (code == 0x2A || code == 0x36) {
        g_shift_pressed = released ? 0 : 1;
        return 0;
    }
 
    if (released) return 0;
 
    return g_shift_pressed ? keyboard_map_shift[code] : keyboard_map[code];
}

/*-- KERNEL MAIN --*/
void kernel_main() {
    clear_screen();
    tasks_init();
    idt_init();
    irq_install_handler(5, sb16_irq5_handler);
    irq_clear_mask(5);
    print(aira_os);
    print("\n BOOTING:AiraOS v3.0 \n");
    if (reset_sb16()) {
        print("[OK]Sound Device Successfully initilazed\n");

    } else {
        print("[ERROR]Sound Device initilaze failed\n");
    }
    if (fat32_init(0)) {
        print("[OK]FAT32 partition mounted\n");
    } else {
        print("[ERROR]FAT32 mount failed\n");
    }
    for(volatile int i=0; i<1000000000; i++);
    print("BOOTING:Initialized ALL \n");
    print("BOOTING:Successfully Booted \n");
    print("BOOTING:ALL OK \n");
    for(volatile int i=0; i<1000000000; i++);  
    clear_screen();
    print(aira_os);
    draw_status_bar();
    print("Welcome To AiraOS!\n");
    print("Live-Session-User$ ");
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
    outb(0x60, 0xED); 
    outb(0x60, 0x07);
    outb(0x0A, 0x01);
    play_wav_file("startup.wav");
    char cmd[64]; int idx = 0;
    while(1) {
    char c = poll_keyboard();
    if (c!=0) {
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
                    else if (compare_string(cmd, "color a")) current_color = 0x339933;
                    else if (compare_string(cmd, "color b")) current_color = 0xFF3333;
                    else if (compare_string(cmd, "color c")) current_color = 0x66FFCC;
                    else if (compare_string(cmd, "color 7")) current_color = 0xFFFFCC;
                    else if (compare_string(cmd, "beep")) {beep();}
                    else if (compare_string(cmd, "aira")) {
                        play_wav_file("snowy.wav"); 
                        clear_screen();
                        draw_bmp(0, 0, 1024, 768, "snowy.bmp");
                        while(1) {
                          char k = poll_keyboard();
                          if (k == 27 || k == 'q') break;
                        }

                        clear_screen();                        
                    }
                    else if (compare_string(cmd,"testpanic")) panic("This is test panic.");
                    else if (compare_string(cmd,"reboot")) reboot();
                    else if (compare_string(cmd,"stopsound")) reset_sb16();
                    else if (compare_string(cmd, "ls")) fat32_list_current_directory();
                    else if (starts_with(cmd, "view ")) {
                            char* target_file = &cmd[5];
                            clear_screen();
                            draw_bmp(0, 0, 1024, 768, target_file);
                            while(1) {
                             char k = poll_keyboard();
                             if (k == 27 || k == 'q') break;
                            }

                            clear_screen();
                    }
                    else if (starts_with(cmd, "asm ")) {
                        //nm -g kernel.bin | grep "func"
                        char* args = &cmd[4];

                        char srcname[16]; int sp = 0;
                        while (args[sp] != ' ' && args[sp] != '\0' && sp < 15) { srcname[sp] = args[sp]; sp++; }
                        srcname[sp] = '\0';
                        char* outname = (args[sp] == ' ') ? &args[sp + 1] : "out.bin";
                        int found = -1;
                        for (int i = 0; i < 10; i++) {
                            if (files[i].active && compare_string(files[i].name, srcname)) { found = i; break; }
                        }

                        if (found == -1) {
                        print("asm: source file not found (write it first with nano)\n");
                        } else {
                        uint8_t* out; int out_len;
                        if (assemble_source(files[found].content, files[found].size, 0x300000, &out, &out_len)) {
                        if (fat32_write_file(outname, out, out_len)) {
                        print("asm: success -> "); print(outname);
                        print(" ("); print_int(out_len); print(" bytes)\n");
                        } else {
                        print("asm: failed to write to disk\n");
                    }
                }
    }
}
                    else if (starts_with(cmd, "run ")) {
                        char* targetf = &cmd[4];
                        uint8_t* load_addr = (uint8_t*)0x300000;
                        uint32_t sz = fat32_load_file(targetf, load_addr);
                        if (sz > 0) {
                            void (*entry)() = (void (*)())load_addr;
                            entry();
                    }
                    }
                    else if (starts_with(cmd, "cd ")) {
                        char* target = &cmd[3];
                        fat32_change_directory(target);
                    }

                    else if (starts_with(cmd, "mkdir ")) {
                        char* target = &cmd[6];
                        if (fat32_make_directory(target)) {
                            print("dir created.\n");
                        } else {
                        print("mkdir fail.\n");
                      }
                    }

                    else if (starts_with(cmd, "write ")) {
                        char* rest = &cmd[6];
                        char fname[16]; int fp = 0;
                        while (rest[fp] != ' ' && rest[fp] != '\0' && fp < 15) { fname[fp] = rest[fp]; fp++; }
                        fname[fp] = '\0';
                        char* content = (rest[fp] == ' ') ? &rest[fp + 1] : &rest[fp];

                        int len = 0;
                        while (content[len] != '\0') len++;

                        if (fat32_write_file(fname, (uint8_t*)content, len)) {
                            print("Successfully writed to file.\n");
                        } else {
                            print("write failed.\n");
                        }
                    }
                    else if (starts_with(cmd, "cat ")) {
                        char* target_file = &cmd[4];
                        fat32_cat_file(target_file);
                    }
                    else if (starts_with(cmd, "play ")) {
                        char* target_file = &cmd[5];
                        play_wav_file(target_file);
                    }
                    else if (compare_string(cmd, "safemode")) {
                        beep_freq(70);
                        print("GORDON GET AWAY FROM THE MACHINE \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("Shutting down \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("Attempting shutdown \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("Is not is not shutting down i- is not \n");
                        for(volatile int i=0; i<10000000; i++);
                        print("AAAAAAAAAA \n");
                        for(volatile int i=0; i<100000000; i++);
                        panic("My God,What are you doing");
                        volatile int a = 5;
                        volatile int b = 0;
                        int c = a / b;
                        void (*crash_ptr)() = (void (*)())0x12345678;
                        crash_ptr();
                        unsigned char *ptr = (unsigned char *)0x0;
                        while(1) {
                            *ptr = 0;
                            ptr++;
                        }
                        
                    }
                    else if (idx > 0) print("Undefined Command\n");
                    
                    idx = 0; print("Live-Session-User$ ");
                } else if (c == '\b' && idx > 0) {
                    idx--; put_char('\b');
                } else if (c != 0 && idx < 63) {
                    cmd[idx++] = c;
                    put_char(c);
                }
            }
        }
    }
