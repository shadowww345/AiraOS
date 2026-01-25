#ifndef AIRA_LANG_H
#define AIRA_LANG_H

typedef enum { TYPE_INT, TYPE_STRING, TYPE_FLOAT } var_type;
extern struct Variable {
    char name[16];
    var_type type;
    int ival;
    float fval;    
    char sval[64];
    int active;
};

int compare_string(char* s1, char* s2);
int starts_with(char* str, char* prefix);
void sleep(int duration);
int find_var(char* name);
void get_string(char* buffer, int max_len);
void process_math(int op, int *i);
void run_interper();
void run_nano();



#endif