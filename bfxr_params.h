#ifndef BFXR_PARAMS_H
#define BFXR_PARAMS_H

#include <stddef.h>

#define NUM_WAVES 12
#define NUM_PARAMS 34
#define SAMPLE_RATE 44100

extern const char* WAVE_NAMES[NUM_WAVES];

typedef enum {
    WAVE_SQUARE = 0,
    WAVE_SAW,
    WAVE_SINE,
    WAVE_NOISE,
    WAVE_TRIANGLE,
    WAVE_PINKNOISE,
    WAVE_TAN,
    WAVE_WHISTLE,
    WAVE_BREAKER,
    WAVE_BITNOISE,
    WAVE_NEW1,
    WAVE_BUZZ
} WaveType;

typedef struct {
    double min;
    double max;
    double def;
} ParamRange;

extern const char* PARAM_NAMES[NUM_PARAMS];
extern const ParamRange PARAM_RANGES[NUM_PARAMS];

typedef struct {
    const char* label;
    int start;
    int end;
} ParamGroup;

extern const ParamGroup PARAM_GROUPS[];
extern const int NUM_GROUPS;

void params_make_default(double params[NUM_PARAMS]);
void params_clamp(int i, double* v);
void params_randomize(double params[NUM_PARAMS]);
void params_blend(const double pl[NUM_PARAMS], const double pr[NUM_PARAMS], double t, double result[NUM_PARAMS]);
double params_to_t(int i, double v);
double t_to_param(int i, double t);
const char* params_display(int i, double v);

#endif
