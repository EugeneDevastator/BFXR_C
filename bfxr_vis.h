#ifndef BFXR_VIS_H
#define BFXR_VIS_H

#include "bfxr_generator.h"
#include "raylib.h"

void vis_draw_waveform(const BfxrWave* wave, int x, int y, int w, int h);
void vis_draw_spectrogram_full(const BfxrWave* wave, int x, int y, int w, int h);
void vis_clear_cache(void);
void vis_set_gradient(const float t[3], const float r[3], const float g[3], const float b[3]);
unsigned long long simple_hash(const void* data, int len);

#endif
