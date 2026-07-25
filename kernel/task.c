#include <kernel.h>
#include <memory.h>
#include <task.h>
#include <graphics.h>

extern void task_switch(uint32_t *old_esp_store, uint32_t new_esp);

static task_t tasks[MAX_TASKS];
static int current_task = -1;
static int task_count = 0;

static uint32_t build_initial_stack(uint8_t* stack_top, void (*entry)()) {
    uint32_t* sp = (uint32_t*)stack_top;

    *(--sp) = (uint32_t)entry;
    *(--sp) = 0x00000202;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    return (uint32_t)sp;
}

void tasks_init() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].id = i;
    }
    current_task = -1;
    task_count = 0;
}

int task_create(void (*entry)()) {
    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) { slot = i; break; }
    }
    if (slot == -1) {
        print("task_create: no free slot\n");
        return -1;
    }

    uint8_t* stack = (uint8_t*)malloc(TASK_STACK_SIZE);
    if (!stack) {
        print("task_create: not enough memory for stack\n");
        return -1;
    }

    tasks[slot].stack_base = stack;
    tasks[slot].esp        = build_initial_stack(stack + TASK_STACK_SIZE, entry);
    tasks[slot].state      = TASK_READY;
    task_count++;

    return slot;
}

static int find_next_task() {
    if (task_count == 0) return -1;
    int start = (current_task == -1) ? 0 : current_task;
    for (int step = 1; step <= MAX_TASKS; step++) {
        int idx = (start + step) % MAX_TASKS;
        if (tasks[idx].state == TASK_READY || tasks[idx].state == TASK_RUNNING) {
            return idx;
        }
    }
    return -1;
}

void yield() {
    int next = find_next_task();
    if (next == -1 || next == current_task) {
        return;
    }

    int prev = current_task;
    if (prev != -1 && tasks[prev].state == TASK_RUNNING) {
        tasks[prev].state = TASK_READY;
    }
    tasks[next].state = TASK_RUNNING;
    current_task = next;

    if (prev == -1) {
        uint32_t dummy;
        task_switch(&dummy, tasks[next].esp);
    } else {
        task_switch(&tasks[prev].esp, tasks[next].esp);
    }
}

void task_exit() {
    if (current_task != -1) {
        tasks[current_task].state = TASK_DEAD;
        free(tasks[current_task].stack_base);
        task_count--;
    }
    for (;;) {
        yield();
    }
}

int task_current_id() {
    return current_task;
}
