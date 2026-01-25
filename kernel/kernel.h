#ifndef KERNEL_H
#define KERNEL_H

typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;


extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char val);
extern int file_size;
extern char file_system_buffer[4096]; 
extern struct File {
    char name[32];
    char content[1024];
    int size;
    int active;
};
extern unsigned char keyboard_map[128];
extern struct File files[10];

extern uint32_t free_mem_addr;
extern uint32_t mem_limit;

void reboot();
void* malloc(int size);
void free(void* ptr);
void panic(const char* messg);



#endif