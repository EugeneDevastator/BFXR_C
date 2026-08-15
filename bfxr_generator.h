#ifndef BFXR_GENERATOR_H
#define BFXR_GENERATOR_H

#include <stdint.h>
#include <stddef.h>
#include "bfxr_params.h"

#define BFXR_SAMPLE_RATE 44100

typedef struct {
    int num_samples;
    int16_t* samples;
} BfxrWave;

void bfxr_wave_free(BfxrWave* wave);

BfxrWave bfxr_generate_wave(const double params[NUM_PARAMS]);
BfxrWave bfxr_generate_wave_blended(const double params[NUM_PARAMS], int wave_type_a, int wave_type_b, double blend_t);

#endif
