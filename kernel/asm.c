#include <kernel.h>
#include <graphics.h>
#include <aira_lang.h>
#include <asm.h>
#include <sound.h>
#include <stdint.h>

#define MAX_LABELS   32
#define MAX_LINE     128
#define MAX_OUT      1024

typedef struct {
    char name[16];
    uint32_t addr;
} label_t;

static uint8_t  g_asm_out[MAX_OUT];
static label_t  g_labels[MAX_LABELS];
static int      g_label_count;

static int strlen_local(const char* s) {
    int i = 0;
    while (s[i] != '\0') i++;
    return i;
}

static void copy_str(char* dst, const char* src, int maxlen) {
    int i = 0;
    while (src[i] != '\0' && i < maxlen - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static void trim(char* s) {
    int start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;
    int i = 0;
    while (s[start + i] != '\0') { s[i] = s[start + i]; i++; }
    s[i] = '\0';
    while (i > 0 && (s[i-1] == ' ' || s[i-1] == '\t' || s[i-1] == '\r' || s[i-1] == '\n')) {
        i--; s[i] = '\0';
    }
}

static void strip_comment(char* s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ';') { s[i] = '\0'; return; }
    }
}

static int parse_number(const char* s, uint32_t* out) {
    int i = 0;
    uint32_t val = 0;
    if (s[0] == '\0') return 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        i = 2;
        if (s[i] == '\0') return 0;
        while (s[i] != '\0') {
            char c = s[i]; int d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else return 0;
            val = val * 16 + d;
            i++;
        }
        *out = val;
        return 1;
    }
    while (s[i] != '\0') {
        if (s[i] < '0' || s[i] > '9') return 0;
        val = val * 10 + (s[i] - '0');
        i++;
    }
    *out = val;
    return 1;
}

static int reg_from_name(const char* s) {
    static const char* names[8] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
    for (int i = 0; i < 8; i++) {
        if (compare_string((char*)s, (char*)names[i])) return i;
    }
    return -1;
}

static int resolve_value(const char* tok, uint32_t* out) {
    if (parse_number(tok, out)) return 1;
    for (int i = 0; i < g_label_count; i++) {
        if (compare_string((char*)tok, g_labels[i].name)) {
            *out = g_labels[i].addr;
            return 1;
        }
    }
    return 0;
}

static int get_line(const char* src, int src_len, int* pos, char* out, int maxlen) {
    if (*pos >= src_len) return 0;
    int i = 0;
    while (*pos < src_len && src[*pos] != '\n') {
        if (i < maxlen - 1) out[i++] = src[*pos];
        (*pos)++;
    }
    out[i] = '\0';
    if (*pos < src_len && src[*pos] == '\n') (*pos)++;
    return 1;
}

static void copy_range_trim(char* dst, const char* src, int start, int end, int maxlen) {
    while (start < end && (src[start] == ' ' || src[start] == '\t')) start++;
    while (end > start && (src[end-1] == ' ' || src[end-1] == '\t')) end--;
    int n = end - start;
    if (n > maxlen - 1) n = maxlen - 1;
    for (int i = 0; i < n; i++) dst[i] = src[start + i];
    dst[n] = '\0';
}

static void split_line(char* line, char* mnemonic, char* op1, char* op2, int* opcount, char* restbuf) {
    int i = 0, m = 0;
    while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t' && m < 15) mnemonic[m++] = line[i++];
    mnemonic[m] = '\0';
    while (line[i] == ' ' || line[i] == '\t') i++;

    int r = 0;
    while (line[i] != '\0' && r < 99) restbuf[r++] = line[i++];
    restbuf[r] = '\0';

    op1[0] = '\0'; op2[0] = '\0'; *opcount = 0;
    int len = strlen_local(restbuf);
    int p = 0;
    while (p < len && restbuf[p] != ',') p++;
    copy_range_trim(op1, restbuf, 0, p, 32);
    if (strlen_local(op1) > 0) *opcount = 1;
    if (p < len && restbuf[p] == ',') {
        p++;
        copy_range_trim(op2, restbuf, p, len, 32);
        if (strlen_local(op2) > 0) *opcount = 2;
    }
}

static int process_db(const char* rest, int emit, uint8_t* out_buf, int* out_len) {
    int count = 0;
    int i = 0;
    while (rest[i] != '\0') {
        while (rest[i] == ' ' || rest[i] == '\t' || rest[i] == ',') i++;
        if (rest[i] == '\0') break;

        if (rest[i] == '"') {
            i++;
            while (rest[i] != '"' && rest[i] != '\0') {
                if (emit) out_buf[(*out_len)++] = (uint8_t)rest[i];
                count++;
                i++;
            }
            if (rest[i] == '"') i++;
        } else {
            char numbuf[16]; int np = 0;
            while (rest[i] != ',' && rest[i] != ' ' && rest[i] != '\t' && rest[i] != '\0' && np < 15) {
                numbuf[np++] = rest[i]; i++;
            }
            numbuf[np] = '\0';
            uint32_t val;
            if (np > 0 && parse_number(numbuf, &val)) {
                if (emit) out_buf[(*out_len)++] = (uint8_t)(val & 0xFF);
                count++;
            }
        }
    }
    return count;
}

static int process_line(char* mnemonic, char* op1, char* op2, int opcount, char* restbuf,int emit, uint8_t* out_buf, int* out_len, uint32_t cur_addr) {

    if (compare_string(mnemonic, "db")) {
        return process_db(restbuf, emit, out_buf, out_len);
    }

    if (compare_string(mnemonic, "mov")) {
        if (opcount != 2) return -1;
        int reg = reg_from_name(op1);
        if (reg < 0) return -1;
        if (emit) {
            uint32_t val;
            if (!resolve_value(op2, &val)) return -1;
            out_buf[(*out_len)++] = (uint8_t)(0xB8 + reg);
            out_buf[(*out_len)++] = (uint8_t)(val & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((val >> 8) & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((val >> 16) & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((val >> 24) & 0xFF);
        }
        return 5;
    }

    if (compare_string(mnemonic, "push")) {
        if (opcount != 1) return -1;
        int reg = reg_from_name(op1);
        if (reg >= 0) {
            if (emit) out_buf[(*out_len)++] = (uint8_t)(0x50 + reg);
            return 1;
        }
        if (emit) {
            uint32_t val;
            if (!resolve_value(op1, &val)) return -1;
            out_buf[(*out_len)++] = 0x68;
            out_buf[(*out_len)++] = (uint8_t)(val & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((val >> 8) & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((val >> 16) & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((val >> 24) & 0xFF);
        }
        return 5;
    }

    if (compare_string(mnemonic, "call")) {
        if (opcount != 1) return -1;
        int reg = reg_from_name(op1);
        if (reg < 0) return -1;
        if (emit) {
            out_buf[(*out_len)++] = 0xFF;
            out_buf[(*out_len)++] = (uint8_t)(0xD0 + reg);
        }
        return 2;
    }

    if (compare_string(mnemonic, "add") || compare_string(mnemonic, "sub")) {
        if (opcount != 2) return -1;
        if (!compare_string(op1, "esp")) return -1;
        if (emit) {
            uint32_t val;
            if (!resolve_value(op2, &val)) return -1;
            out_buf[(*out_len)++] = 0x83;
            out_buf[(*out_len)++] = compare_string(mnemonic, "add") ? 0xC4 : 0xEC;
            out_buf[(*out_len)++] = (uint8_t)(val & 0xFF);
        }
        return 3;
    }

    if (compare_string(mnemonic, "int")) {
        if (opcount != 1) return -1;
        if (emit) {
            uint32_t val;
            if (!resolve_value(op1, &val)) return -1;
            out_buf[(*out_len)++] = 0xCD;
            out_buf[(*out_len)++] = (uint8_t)(val & 0xFF);
        }
        return 2;
    }

    if (compare_string(mnemonic, "jmp")) {
        if (opcount != 1) return -1;
        if (emit) {
            uint32_t target;
            if (!resolve_value(op1, &target)) return -1;
            int32_t rel = (int32_t)(target - (cur_addr + 5));
            uint32_t relu = (uint32_t)rel;
            out_buf[(*out_len)++] = 0xE9;
            out_buf[(*out_len)++] = (uint8_t)(relu & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((relu >> 8) & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((relu >> 16) & 0xFF);
            out_buf[(*out_len)++] = (uint8_t)((relu >> 24) & 0xFF);
        }
        return 5;
    }

    if (compare_string(mnemonic, "ret")) { if (emit) out_buf[(*out_len)++] = 0xC3; return 1; }
    if (compare_string(mnemonic, "hlt")) { if (emit) out_buf[(*out_len)++] = 0xF4; return 1; }
    if (compare_string(mnemonic, "nop")) { if (emit) out_buf[(*out_len)++] = 0x90; return 1; }
    if (compare_string(mnemonic, "cli")) { if (emit) out_buf[(*out_len)++] = 0xFA; return 1; }
    return -1;
}

static int extract_label(char* line, char* label_out) {
    int i = 0;
    while (line[i] != '\0' && line[i] != ':') i++;
    if (line[i] != ':') return 0;

    int n = i;
    if (n > 15) n = 15;
    for (int j = 0; j < n; j++) label_out[j] = line[j];
    label_out[n] = '\0';
    for (int j = 0; j < n; j++) {
        if (label_out[j] == ' ' || label_out[j] == '\t') return 0;
    }

    int k = 0, src_i = i + 1;
    while (line[src_i] != '\0') { line[k++] = line[src_i++]; }
    line[k] = '\0';
    trim(line);
    return 1;
}

int assemble_source(const char* src, int src_len, uint32_t org_addr, uint8_t** out_ptr, int* out_len) {
    g_label_count = 0;

    char line[MAX_LINE];
    char mnemonic[16], op1[32], op2[32], restbuf[100];
    int opcount;

    uint32_t addr = org_addr;
    int pos = 0;
    int line_no = 0;
    while (get_line(src, src_len, &pos, line, MAX_LINE)) {
        line_no++;
        strip_comment(line);
        trim(line);
        if (line[0] == '\0') continue;

        char label_name[16];
        if (extract_label(line, label_name)) {
            if (g_label_count >= MAX_LABELS) {
                print("asm: too many labels\n");
                return 0;
            }
            copy_str(g_labels[g_label_count].name, label_name, 16);
            g_labels[g_label_count].addr = addr;
            g_label_count++;
            if (line[0] == '\0') continue;
        }

        split_line(line, mnemonic, op1, op2, &opcount, restbuf);
        int size = process_line(mnemonic, op1, op2, opcount, restbuf, 0, 0, 0, addr);
        if (size < 0) {
            print("asm: line "); print_int(line_no); print(": invalid instruction -> ");
            print(mnemonic); print("\n");
            return 0;
        }
        addr += size;
    }

    uint32_t cur = org_addr;
    int len = 0;
    pos = 0;
    line_no = 0;
    while (get_line(src, src_len, &pos, line, MAX_LINE)) {
        line_no++;
        strip_comment(line);
        trim(line);
        if (line[0] == '\0') continue;

        char label_name[16];
        if (extract_label(line, label_name)) {
            if (line[0] == '\0') continue;
        }

        split_line(line, mnemonic, op1, op2, &opcount, restbuf);

        if (len + 8 > MAX_OUT) {
            print("asm: output buffer full (MAX_OUT exceeded)\n");
            return 0;
        }

        int size = process_line(mnemonic, op1, op2, opcount, restbuf, 1, g_asm_out, &len, cur);
        if (size < 0) {
            print("asm: line "); print_int(line_no); print(": could not resolve address/label\n");
            return 0;
        }
        cur += size;
    }

    *out_ptr = g_asm_out;
    *out_len = len;
    return 1;
}
