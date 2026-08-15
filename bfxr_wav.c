#include "bfxr_wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_fmt;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} WavHeader;
#pragma pack(pop)

int bfxr_wav_save(const char* filename, const BfxrWave* wave) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return -1;

    int num_samples = wave->num_samples;
    int data_size = num_samples * sizeof(int16_t);
    int file_size = 36 + data_size;

    WavHeader hdr;
    memcpy(hdr.riff, "RIFF", 4);
    hdr.file_size = file_size;
    memcpy(hdr.wave, "WAVE", 4);
    memcpy(hdr.fmt, "fmt ", 4);
    hdr.fmt_size = 16;
    hdr.audio_fmt = 1;
    hdr.num_channels = 1;
    hdr.sample_rate = 44100;
    hdr.byte_rate = 44100 * sizeof(int16_t);
    hdr.block_align = sizeof(int16_t);
    hdr.bits_per_sample = 16;
    memcpy(hdr.data, "data", 4);
    hdr.data_size = data_size;

    fwrite(&hdr, sizeof(hdr), 1, fp);
    fwrite(wave->samples, sizeof(int16_t), num_samples, fp);
    fclose(fp);
    return 0;
}
