#ifndef SOUND_H
#define SOUND_H

typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

//\\ --Sound Blaster 16-- \\//
int reset_sb16();
void sb16_write_dsp(uint8_t data);
extern uint8_t sb16_read_dsp();
void setup_dma(uint32_t buffer_addr, uint32_t length);

//\\ --PC Speaker-- \\//
void play_sound(unsigned int nFrequence);
void stop_sound();
void beep();
void beep_with_duration(unsigned int freq, int duration);
void beep_freq(unsigned int freq);


#endif