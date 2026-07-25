#include <kernel.h>
#include <memory.h>
#include <graphics.h>

typedef struct heap_block {
    uint32_t size;
    uint8_t  is_free;
    struct heap_block* next;
} heap_block_t;

#define HEAP_ALIGN 4
#define MIN_SPLIT_REMAINDER (sizeof(heap_block_t) + 16)

static heap_block_t* g_heap_start = 0;

void heap_init() {
    g_heap_start = (heap_block_t*)free_mem_addr;
    g_heap_start->size    = mem_limit - free_mem_addr - sizeof(heap_block_t);
    g_heap_start->is_free = 1;
    g_heap_start->next    = 0;
}

static uint32_t align_up(uint32_t n, uint32_t a) {
    return (n + (a - 1)) & ~(a - 1);
}

void* malloc(uint32_t size) {
    if (!g_heap_start) heap_init();
    if (size == 0) return 0;
    size = align_up(size, HEAP_ALIGN);

    heap_block_t* blk = g_heap_start;
    while (blk) {
        if (blk->is_free && blk->size >= size) {
            if (blk->size >= size + MIN_SPLIT_REMAINDER) {
                heap_block_t* new_blk = (heap_block_t*)((uint8_t*)blk + sizeof(heap_block_t) + size);
                new_blk->size    = blk->size - size - sizeof(heap_block_t);
                new_blk->is_free = 1;
                new_blk->next    = blk->next;

                blk->size = size;
                blk->next = new_blk;
            }
            blk->is_free = 0;
            return (void*)((uint8_t*)blk + sizeof(heap_block_t));
        }
        blk = blk->next;
    }

    print("malloc: out of memory\n");
    return 0;
}

void free(void* ptr) {
    if (!ptr) return;
    heap_block_t* blk = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    blk->is_free = 1;

    while (blk->next && blk->next->is_free) {
        blk->size += sizeof(heap_block_t) + blk->next->size;
        blk->next = blk->next->next;
    }
}

int memcmp(const void* s1, const void* s2, uint32_t n) {
    const uint8_t *p1 = s1, *p2 = s2;
    for (uint32_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}
