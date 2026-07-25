#include <kernel.h>
#include <idt.h>
#include <pit.h>

#define PIT_BASE_FREQ 1193182u
#define PIT_CMD  0x43
#define PIT_CH0  0x40

static volatile uint32_t g_ticks = 0;

static void pit_irq0_handler() {
    g_ticks++;
}

void pit_init() {
    uint32_t divisor = PIT_BASE_FREQ / PIT_HZ;

    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    g_ticks = 0;
    irq_install_handler(0, pit_irq0_handler);
    irq_clear_mask(0);
}

uint32_t get_ticks_ms() {
    return g_ticks * (1000 / PIT_HZ);
}

void sleep_ms(uint32_t ms) {
    uint32_t target = get_ticks_ms() + ms;
    while (get_ticks_ms() < target) {
        __asm__ __volatile__("hlt");
    }
}
