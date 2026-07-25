#ifndef SOUND_H
#define SOUND_H

typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;
typedef signed int   int32_t;
typedef signed short int16_t;
typedef signed char  int8_t;

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

int  reset_sb16(void);
void sb16_write_dsp(uint8_t data);
uint8_t sb16_read_dsp(void);
void sb16_irq5_handler(void);
void sb16_set_format(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channels);

void prepare_audio_cluster(uint32_t start_cluster, uint32_t file_size);
void play_wav(uint8_t* wav_data);
void play_wav_stream_cluster(uint32_t start_cluster, uint32_t file_size);
void play_wav_file(const char* filename);

void play_sound(unsigned int nFrequence);
void stop_sound(void);
void beep(void);
void beep_with_duration(int freq, int duration);
void beep_freq(int freq);

#endif