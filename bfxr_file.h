#ifndef BFXR_FILE_H
#define BFXR_FILE_H

#include "bfxr_params.h"
#include "bfxr_generator.h"

int bfxr_save_preset(const char* filename, const double params[NUM_PARAMS]);
int bfxr_load_preset(const char* filename, double params[NUM_PARAMS]);

int bfxr_save_scene(const char* filename, const double params_l[NUM_PARAMS], const double params_r[NUM_PARAMS], double blend_t);
int bfxr_load_scene(const char* filename, double params_l[NUM_PARAMS], double params_r[NUM_PARAMS], double* blend_t);

int bfxr_wav_save(const char* filename, const BfxrWave* wave);

#endif
