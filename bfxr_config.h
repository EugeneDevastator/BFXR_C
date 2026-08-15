#ifndef BFXR_CONFIG_H
#define BFXR_CONFIG_H

#include "bfxr_params.h"

typedef struct {
    float volume;
    int autoplay;
    float grad_t[3];
    float grad_r[3];
    float grad_g[3];
    float grad_b[3];
} BfxrConfig;

void config_load(BfxrConfig* cfg);
void config_save(const BfxrConfig* cfg);

void config_load_scene(double params_l[NUM_PARAMS], double params_r[NUM_PARAMS], double* blend_t);
void config_save_scene(const double params_l[NUM_PARAMS], const double params_r[NUM_PARAMS], double blend_t);

#endif
