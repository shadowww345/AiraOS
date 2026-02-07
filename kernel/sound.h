#ifndef SOUND_H
#define SOUND_H

typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;
extern uint8_t* sound_buffer;
typedef struct {
    char     riff[4];
    uint32_t overall_size;
    char     wave[4];
    char     fmt_chunk_marker[4];
    uint32_t length_of_fmt;
    uint16_t format_type;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data_chunk_header[4];
    uint32_t data_size;
} __attribute__((packed)) WAV_Header;

//\\ --Sound Blaster 16-- \\//
int reset_sb16();
void sb16_write_dsp(uint8_t data);
extern uint8_t sb16_read_dsp();
void setup_dma(uint32_t buffer_addr, uint32_t length);

//\\ --PC Speaker-- \\//
void play_sound(unsigned int nFrequence);
void stop_sound();
void beep();
void beep_with_duration(int freq, int duration);
void beep_freq(int freq);
void prepare_audio();
void play_wav(uint8_t* wav_data);

#endif
