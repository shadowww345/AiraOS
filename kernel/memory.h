#ifndef MEMORY_H
#define MEMORY_H
#include <kernel.h>

void heap_init();
void* malloc(uint32_t size);
void  free(void* ptr);


int memcmp(const void* s1, const void* s2, uint32_t n);


#endif