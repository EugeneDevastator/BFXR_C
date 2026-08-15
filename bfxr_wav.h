#ifndef BFXR_WAV_H
#define BFXR_WAV_H

#include "bfxr_generator.h"
#include <stddef.h>

int bfxr_wav_save(const char* filename, const BfxrWave* wave);

#endif
