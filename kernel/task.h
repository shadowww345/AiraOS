#ifndef TASK_H
#define TASK_H

#include <kernel.h>

#define MAX_TASKS      8
#define TASK_STACK_SIZE 4096

typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_DEAD
} task_state_t;

typedef struct task {
    uint32_t      esp;
    uint8_t*      stack_base;
    task_state_t  state;
    int           id;
} task_t;

void tasks_init();
int task_create(void (*entry)());
void yield();
void task_exit();
int task_current_id();

#endif
