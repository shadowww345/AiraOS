#include <kernel.h>
#include <sound.h>
#include <fat32.h>
#include <idt.h>
#include <graphics.h>
#include <task.h>
extern void read_sector(uint32_t lba, uint8_t *buffer);

#define DMA_CHUNK_SIZE 0x8000

#define SB16_PORT_BASE 0x220
#define SB16_RESET     (SB16_PORT_BASE + 0x6)
#define SB16_READ      (SB16_PORT_BASE + 0xA)
#define SB16_WRITE     (SB16_PORT_BASE + 0xC)
#define SB16_STATUS    (SB16_PORT_BASE + 0xE)
#define SB16_ACK8      (SB16_PORT_BASE + 0xE)
#define SB16_ACK16     (SB16_PORT_BASE + 0xF)

typedef struct {
    uint32_t cur_cluster;
    uint32_t sector_in_cluster;
    uint32_t remaining;
    uint32_t buf_offset;
    uint8_t  playing;
    uint8_t  bits_per_sample;
    uint8_t  channels;
    uint8_t  cur_buf;
    uint32_t next_len;
} sb16_stream_t;

static volatile sb16_stream_t stream;

uint8_t* sound_buffer = (uint8_t*)0x20000;
#define CHUNK_BUF sound_buffer

static inline uint8_t* buf_ptr(uint8_t idx) {
    return sound_buffer + (idx ? DMA_CHUNK_SIZE : 0);
}

static void read_stream_sectors(uint8_t* dst, uint32_t sectors_needed) {
    uint32_t spc = fat32_cluster_to_lba(3) - fat32_cluster_to_lba(2);
    if (spc == 0) spc = 8;

    for (uint32_t s = 0; s < sectors_needed; s++) {
        if (stream.cur_cluster < 2 || stream.cur_cluster >= FAT32_EOC_MIN) {
            for (uint32_t b = 0; b < 512; b++) {
                dst[s * 512 + b] = 0;
            }
            continue;
        }

        uint32_t lba = fat32_cluster_to_lba(stream.cur_cluster) + stream.sector_in_cluster;
        read_sector(lba, dst + (s * 512));

        stream.sector_in_cluster++;
        if (stream.sector_in_cluster >= spc) {
            stream.sector_in_cluster = 0;
            stream.cur_cluster = fat32_get_next_cluster(stream.cur_cluster);
        }
    }
}

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

static int find_data_chunk(uint8_t* buf, uint32_t buf_limit) {
    uint32_t pos = 12;
    while (pos + 8 <= buf_limit) {
        char* chunk_id = (char*)(buf + pos);
        uint32_t chunk_size = *(uint32_t*)(buf + pos + 4);
        if (chunk_id[0]=='d' && chunk_id[1]=='a' && chunk_id[2]=='t' && chunk_id[3]=='a') {
            return (int)pos;
        }
        pos += 8 + chunk_size;
        if (chunk_size % 2 == 1) pos++;
    }
    return -1;
}

static uint32_t setup_dma_chunk(uint32_t addr, uint32_t length, uint8_t frame_size) {
    if ((addr & 0xFFFF) + length > 0x10000) {
        length = 0x10000 - (addr & 0xFFFF);
        length -= (length % frame_size);
    }

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
    return length;
}

static uint32_t setup_dma_chunk16(uint32_t addr, uint32_t length, uint8_t frame_size) {
    length &= ~1u;

    uint32_t page_off = addr & 0x1FFFF;
    if (page_off + length > 0x20000) {
        length = 0x20000 - page_off;
        length &= ~1u;
    }
    length -= (length % frame_size);

    uint16_t word_addr  = (uint16_t)((addr >> 1) & 0xFFFF);
    uint8_t  page        = (uint8_t)((addr >> 16) & 0xFF);
    uint16_t word_count  = (uint16_t)((length / 2) - 1);

    outb(0xD4, 0x05);
    outb(0xD8, 0x00);
    outb(0xD6, 0x49);

    outb(0xC4, (uint8_t)(word_addr & 0xFF));
    outb(0xC4, (uint8_t)((word_addr >> 8) & 0xFF));
    outb(0x8B, page);

    outb(0xC6, (uint8_t)(word_count & 0xFF));
    outb(0xC6, (uint8_t)((word_count >> 8) & 0xFF));

    outb(0xD4, 0x01);
    return length;
}

static uint8_t sb16_mode_byte(uint8_t bits_per_sample, uint8_t channels) {
    uint8_t mode = 0;
    if (bits_per_sample == 16) mode |= 0x20;
    if (channels == 2)         mode |= 0x10;
    return mode;
}

void sb16_set_format(uint32_t sample_rate, uint8_t bits_per_sample, uint8_t channels) {
    sb16_write_dsp(0x41);
    sb16_write_dsp((uint8_t)((sample_rate >> 8) & 0xFF));
    sb16_write_dsp((uint8_t)(sample_rate & 0xFF));
}

static uint32_t sb16_start_chunk_fmt(uint32_t addr, uint32_t length, uint8_t bits_per_sample, uint8_t channels) {
    uint8_t mode = sb16_mode_byte(bits_per_sample, channels);
    uint8_t frame_size = channels * (bits_per_sample / 8);
    uint32_t actual;

    if (bits_per_sample == 16) {
        actual = setup_dma_chunk16(addr, length, frame_size);
        uint16_t sample_count = (uint16_t)((actual / 2) - 1);
        sb16_write_dsp(0xB0);
        sb16_write_dsp(mode);
        sb16_write_dsp((uint8_t)(sample_count & 0xFF));
        sb16_write_dsp((uint8_t)((sample_count >> 8) & 0xFF));
    } else {
        actual = setup_dma_chunk(addr, length, frame_size);
        uint16_t sample_count = (uint16_t)(actual - 1);
        sb16_write_dsp(0xC0);
        sb16_write_dsp(mode);
        sb16_write_dsp((uint8_t)(sample_count & 0xFF));
        sb16_write_dsp((uint8_t)((sample_count >> 8) & 0xFF));
    }
    return actual;
}

static void sb16_fill_buffer(uint8_t idx) {
    if (stream.remaining == 0) {
        stream.next_len = 0;
        return;
    }
    uint32_t chunk_len = (stream.remaining < DMA_CHUNK_SIZE) ? stream.remaining : DMA_CHUNK_SIZE;
    uint32_t frame_size = stream.channels * (stream.bits_per_sample / 8);
    chunk_len -= (chunk_len % frame_size);

    uint32_t sectors_needed = (chunk_len + 511) / 512;
    uint8_t* dst = buf_ptr(idx);

    read_stream_sectors(dst, sectors_needed);

    stream.remaining -= chunk_len;
    stream.next_len = chunk_len;
}

void sb16_irq5_handler() {
    inb(SB16_ACK8);
    inb(SB16_ACK16);

    if (!stream.playing) return;

    if (stream.next_len == 0) {
        stream.playing = 0;
        return;
    }

    uint8_t play_idx = 1 - stream.cur_buf;
    sb16_start_chunk_fmt((uint32_t)buf_ptr(play_idx), stream.next_len, stream.bits_per_sample, stream.channels);
    stream.cur_buf = play_idx;

    sb16_fill_buffer(1 - play_idx);
}

void prepare_audio_cluster(uint32_t start_cluster, uint32_t file_size) {
    stream.cur_cluster = start_cluster;
    stream.sector_in_cluster = 0;
    uint32_t sectors_needed = (file_size + 511) / 512;
    read_stream_sectors(sound_buffer, sectors_needed);
}

void play_wav_stream_cluster(uint32_t start_cluster, uint32_t file_size) {
    yield();
    stream.cur_cluster = start_cluster;
    stream.sector_in_cluster = 0;

    uint32_t first_chunk_sectors = DMA_CHUNK_SIZE / 512;
    read_stream_sectors(CHUNK_BUF, first_chunk_sectors);

    WAV_Header* riff_header = (WAV_Header*)CHUNK_BUF;

    int data_chunk_pos = find_data_chunk(CHUNK_BUF, DMA_CHUNK_SIZE);
    if (data_chunk_pos < 0) {
        print("play_wav_stream: 'data' chunk not found\n");
        return;
    }
    uint32_t data_size = *(uint32_t*)(CHUNK_BUF + data_chunk_pos + 4);
    uint32_t audio_start = (uint32_t)data_chunk_pos + 8;

    uint8_t bits = riff_header->bits_per_sample;
    uint8_t channels = (uint8_t)riff_header->channels;

    if ((bits != 8 && bits != 16) || (channels != 1 && channels != 2)) {
        print("play_wav_stream: unsupported format\n");
        return;
    }

    sb16_write_dsp(0xD1);
    sb16_set_format(riff_header->sample_rate, bits, channels);

    uint32_t first_audio_avail = (DMA_CHUNK_SIZE > audio_start) ? (DMA_CHUNK_SIZE - audio_start) : 0;
    uint32_t total = (data_size < file_size) ? data_size : file_size;
    uint32_t first_len = (total < first_audio_avail) ? total : first_audio_avail;
    uint32_t frame_size = channels * (bits / 8);
    first_len -= (first_len % frame_size);

    stream.remaining       = total - first_len;
    stream.playing         = 1;
    stream.bits_per_sample = bits;
    stream.channels        = channels;
    stream.cur_buf         = 0;

    for (uint32_t i = 0; i < first_len; i++) {
        CHUNK_BUF[i] = CHUNK_BUF[audio_start + i];
    }

    sb16_start_chunk_fmt((uint32_t)CHUNK_BUF, first_len, bits, channels);
    sb16_fill_buffer(1);
}

void play_wav_file(const char* filename) {
    yield();
    FAT32_DirectoryEntry entry;
    if (!fat32_find(filename, &entry)) {
        print("play_wav_file: file not found\n");
        return;
    }
    uint32_t start_cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    play_wav_stream_cluster(start_cluster, entry.file_size);
}

void play_wav(uint8_t* wav_data) {
    yield();
    WAV_Header* header = (WAV_Header*)wav_data;

    uint8_t bits = header->bits_per_sample;
    uint8_t channels = (uint8_t)header->channels;
    if (bits != 8 && bits != 16) return;
    if (channels != 1 && channels != 2) return;
    if (!reset_sb16()) return;

    if (header->data_size > DMA_CHUNK_SIZE) {
        print("play_wav: file is too big.\n");
        return;
    }

    sb16_write_dsp(0xD1);
    sb16_set_format(header->sample_rate, bits, channels);

    stream.remaining       = 0;
    stream.playing         = 0;
    stream.bits_per_sample = bits;
    stream.channels        = channels;

    sb16_start_chunk_fmt((uint32_t)(wav_data + 44), header->data_size, bits, channels);
}

void play_sound(unsigned int nFrequence) {
    unsigned int Div = 1193180 / nFrequence;
    outb(0x43, 0xB6);
    outb(0x42, (unsigned char)(Div));
    outb(0x42, (unsigned char)(Div >> 8));

    unsigned char tmp = inb(0x61);
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

void beep_with_duration(int freq, int duration) {
    play_sound(freq);
    for(volatile int i = 0; i < duration * 10000000; i++);
    stop_sound();
}
