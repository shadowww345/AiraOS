#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <kernel.h>

int assemble_source(const char* src, int src_len, uint32_t org_addr, uint8_t** out_ptr, int* out_len);

#endif
