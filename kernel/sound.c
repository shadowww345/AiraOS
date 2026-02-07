#include <kernel.h>
#include <sound.h>
#include <fat12.h>

//\\ --Sound Blaster 16-- \\//
#define SB16_PORT_BASE 0x220
#define SB16_RESET     (SB16_PORT_BASE + 0x6)
#define SB16_READ      (SB16_PORT_BASE + 0xA)
#define SB16_WRITE     (SB16_PORT_BASE + 0xC)
#define SB16_STATUS    (SB16_PORT_BASE + 0xE)

int reset_sb16() {
    outb(SB16_RESET, 1);
    for(int i = 0; i < 1000; i++) { asm("nop"); }
    outb(SB16_RESET, 0);
    for(int timeout = 0; timeout < 10000; timeout++) {
        if (inb(SB16_STATUS) & 0x80) {
            if (inb(SB16_READ) == 0xAA) {
                return 1;
            }
        }
    }
    return 0;
}


void sb16_write_dsp(uint8_t data) {
    while (inb(SB16_WRITE) & 0x80);
    outb(SB16_WRITE, data);
}

uint8_t sb16_read_dsp() {
    while (!(inb(SB16_STATUS) & 0x80));
    return inb(SB16_READ);
}

void setup_dma(uint32_t addr, uint32_t length) {
    outb(0x0A, 0x05);
    outb(0x0C, 0x00);
    outb(0x0B, 0x49);

    outb(0x02, (uint8_t)(addr & 0xFF));
    outb(0x02, (uint8_t)((addr >> 8) & 0xFF));
    outb(0x83, (uint8_t)((addr >> 16) & 0xFF));

    uint16_t count = (uint16_t)(length - 1);
    outb(0x03, (uint8_t)(count & 0xFF));
    outb(0x03, (uint8_t)((count >> 8) & 0xFF));

    outb(0x0A, 0x01);
}

uint8_t* sound_buffer = (uint8_t*)0x20000;
void prepare_audio(uint32_t start_lba) {
    for (int i = 0; i < 82; i++) {
        read_sector(start_lba + i, sound_buffer + (i * 512));
    }
}

void play_wav(uint8_t* wav_data) {
    WAV_Header* header = (WAV_Header*)wav_data;

    if (header->bits_per_sample != 8) {
        return; 
    }
    if (!reset_sb16()) return; 
    sb16_write_dsp(0xD1);
    uint8_t time_constant = 256 - (1000000 / header->sample_rate);
    sb16_write_dsp(0x40);
    sb16_write_dsp(time_constant);
    uint32_t audio_data_addr = (uint32_t)wav_data + 44;
    setup_dma(audio_data_addr, header->data_size);
    sb16_write_dsp(0x14);
    sb16_write_dsp((uint8_t)((header->data_size - 1) & 0xFF));
    sb16_write_dsp((uint8_t)(((header->data_size - 1) >> 8) & 0xFF));
}

//\\ --PC Speaker-- \\//

void play_sound(unsigned int nFrequence) {
    unsigned int Div;
    unsigned char tmp;

    Div = 1193180 / nFrequence;
    
    outb(0x43, 0xB6);
    outb(0x42, (unsigned char) (Div) );
    outb(0x42, (unsigned char) (Div >> 8));

    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void stop_sound() {
    unsigned char tmp = (inb(0x61) & 0xFC);
    outb(0x61, tmp);
}

void beep() {
    play_sound(440);
    for(volatile int i = 0; i < 10000000; i++); 
    stop_sound();
}

void beep_freq(int freq) {
    play_sound(freq);
    for(volatile int i = 0; i < 10000000; i++); 
    stop_sound();
}

void beep_with_duration(int freq,int duration) {
    play_sound(freq);
    for(volatile int i = 0; i < duration * 10000000; i++); 
    stop_sound();
}
