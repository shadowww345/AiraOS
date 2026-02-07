
/*
 * [ AiraOS v2.0 - GUI Note ]
 * * I tried to implement a Graphical User Interface (GUI), but it is 
 * quite complex and time-consuming at this stage. 
 *
 * For now, I am focusing on the kernel core and backend logic. 
 * If anyone wants to develop a GUI for AiraOS, feel free to contribute!
 * Maybe ı make little GUI
 */

#include <kernel.h>
#include <graphics.h>
#include <aira_lang.h>
#include <sound.h>
#include <fat12.h>

unsigned int bgcolor = 0xFF0033;
int file_size = 0; 
char file_system_buffer[4096]; 
struct File files[10];
uint32_t free_mem_addr = 0x100000; 
uint32_t mem_limit = 0x200000;
void reboot() {
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    while(1) { __asm__("hlt"); }
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


unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '(', ')', '\n', 
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '#', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '"', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '{', '}', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//Memory//
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

/*-- KERNEL MAIN --*/
void kernel_main() {
    clear_screen();
    print(aira_os);
    print("\n BOOTING:AiraOS v2.0 \n");
    if (reset_sb16()) {
        print("[OK]Sound Device Successfully initilazed\n");

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
    print(aira_os);
    draw_status_bar();
    print("Welcome To AiraOS!\n> ");
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
    outb(0x60, 0xED); 
    outb(0x60, 0x07);
    outb(0x0A, 0x01);
    prepare_audio(500);
    play_wav(sound_buffer);
    char cmd[64]; int idx = 0;
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
                    else if (compare_string(cmd, "color a")) current_color = 0x339933;
                    else if (compare_string(cmd, "color b")) current_color = 0xFF3333;
                    else if (compare_string(cmd, "color c")) current_color = 0x66FFCC;
                    else if (compare_string(cmd, "color 7")) current_color = 0xFFFFCC;
                    else if (compare_string(cmd, "beep")) {beep();}
                    else if (compare_string(cmd,"testpanic")) panic("This is test panic.");
                    else if (compare_string(cmd,"reboot")) reboot();
                    else if (compare_string(cmd,"ls")) list_root_directory();
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






