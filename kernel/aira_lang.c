
#include <aira_lang.h>
#include <kernel.h>
#include <graphics.h>
#include <sound.h>

struct Variable var_table[50];

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

void sleep(int duration) {
    for(volatile int i = 0; i < duration * 1000000; i++) {

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
    sleep(50);
    clear_screen();
    draw_status_bar();
}