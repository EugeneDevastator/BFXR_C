#include "bfxr_params.h"
#include <stdlib.h>
#include <stdio.h>

const char* WAVE_NAMES[NUM_WAVES] = {
    "Square", "Saw", "Sine", "Noise", "Triangle",
    "PinkNoise", "Tan", "Whistle", "Breaker", "Bitnoise", "New1", "Buzz"
};

const char* PARAM_NAMES[NUM_PARAMS] = {
    "WaveType", "Volume",
    "Attack", "Sustain", "SustPunch", "Decay",
    "StartFreq", "MinFreq", "Slide", "DeltaSlide",
    "VibDepth", "VibSpeed",
    "ChgAmt", "ChgSpd", "ChgAmt2", "ChgSpd2", "ChgRepeat",
    "SqDuty", "DutySweep",
    "RepeatSpd",
    "FlngOffset", "FlngSweep",
    "LPCutoff", "LPCutSweep", "LPResonance",
    "HPCutoff", "HPCutSweep",
    "BitCrush", "BitCrushSwp", "Compression",
    "Overtones", "OvtoneFalloff",
    "WaveTypeB", "BlendAmt",
};

const ParamRange PARAM_RANGES[NUM_PARAMS] = {
    [0]  = {0,    11,   2.0},   // WaveType
    [1]  = {0,     1,   0.5},   // Volume
    [2]  = {0,     1,   0.0},   // Attack
    [3]  = {0,     1,   0.3},   // Sustain
    [4]  = {0,     1,   0.0},   // SustPunch
    [5]  = {0,     1,   0.4},   // Decay
    [6]  = {0,     1,   0.3},   // StartFreq
    [7]  = {0,     1,   0.0},   // MinFreq
    [8]  = {-1,    1,   0.0},   // Slide
    [9]  = {-1,    1,   0.0},   // DeltaSlide
    [10] = {0,     1,   0.0},   // VibDepth
    [11] = {0,     1,   0.0},   // VibSpeed
    [12] = {-1,    1,   0.0},   // ChgAmt
    [13] = {0,     1,   0.0},   // ChgSpd
    [14] = {-1,    1,   0.0},   // ChgAmt2
    [15] = {0,     1,   0.0},   // ChgSpd2
    [16] = {0,     1,   0.0},   // ChgRepeat
    [17] = {0,     1,   0.0},   // SqDuty
    [18] = {-1,    1,   0.0},   // DutySweep
    [19] = {0,     1,   0.0},   // RepeatSpd
    [20] = {-1,    1,   0.0},   // FlngOffset
    [21] = {-1,    1,   0.0},   // FlngSweep
    [22] = {0,     1,   1.0},   // LPCutoff
    [23] = {-1,    1,   0.0},   // LPCutSweep
    [24] = {0,     1,   0.0},   // LPResonance
    [25] = {0,     1,   0.0},   // HPCutoff
    [26] = {-1,    1,   0.0},   // HPCutSweep
    [27] = {0,     1,   0.0},   // BitCrush
    [28] = {-1,    1,   0.0},   // BitCrushSwp
    [29] = {0,     1,   0.3},   // Compression
    [30] = {0,     1,   0.0},   // Overtones
    [31] = {0,     1,   0.0},   // OvtoneFalloff
    [32] = {0,    11,   2.0},   // WaveTypeB
    [33] = {-1,    2,   0.0},   // BlendAmt
};

const ParamGroup PARAM_GROUPS[] = {
    {"Wave",       0,  2},
    {"Envelope",   2,  6},
    {"Frequency",  6, 10},
    {"Vibrato",   10, 12},
    {"Change",    12, 17},
    {"Duty",      17, 19},
    {"Repeat",    19, 20},
    {"Flanger",   20, 22},
    {"Filters",   22, 27},
    {"Bit/Comp",  27, 30},
    {"Overtones", 30, 32},
    {"Blend",     32, 34},
};

const int NUM_GROUPS = sizeof(PARAM_GROUPS) / sizeof(PARAM_GROUPS[0]);

void params_make_default(double params[NUM_PARAMS]) {
    for (int i = 0; i < NUM_PARAMS; i++) {
        params[i] = PARAM_RANGES[i].def;
    }
}

void params_clamp(int i, double* v) {
    double lo = PARAM_RANGES[i].min;
    double hi = PARAM_RANGES[i].max;
    if (*v < lo) *v = lo;
    if (*v > hi) *v = hi;
}

void params_randomize(double params[NUM_PARAMS]) {
    for (int i = 0; i < NUM_PARAMS; i++) {
        double lo = PARAM_RANGES[i].min;
        double hi = PARAM_RANGES[i].max;
        params[i] = lo + ((double)rand() / RAND_MAX) * (hi - lo);
    }
    params[0]  = (double)(rand() % NUM_WAVES);
    params[32] = (double)(rand() % NUM_WAVES);

    if (rand() % 2) {
        double r = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        params[6] = r * r;
    } else {
        double r = ((double)rand() / RAND_MAX) * 0.5;
        params[6] = r*r*r + 0.5;
    }
    params[7] = 0.0;

    double r = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    params[8] = r*r*r*r*r;

    r = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    params[9] = r*r*r;

    if (rand() % 2) {
        params[19] = 0.0;
    }
    params[33] = 0.0;
}

void params_blend(const double pl[NUM_PARAMS], const double pr[NUM_PARAMS], double t, double result[NUM_PARAMS]) {
    for (int i = 0; i < NUM_PARAMS; i++) {
        result[i] = pl[i] * (1.0 - t) + pr[i] * t;
    }
}

double params_to_t(int i, double v) {
    double lo = PARAM_RANGES[i].min;
    double hi = PARAM_RANGES[i].max;
    return (v - lo) / (hi - lo);
}

double t_to_param(int i, double t) {
    double lo = PARAM_RANGES[i].min;
    double hi = PARAM_RANGES[i].max;
    double v = lo + t * (hi - lo);
    params_clamp(i, &v);
    return v;
}

const char* params_display(int i, double v) {
    static char buf[32];
    if (i == 0 || i == 32) {
        int wt = (int)v;
        if (wt < 0) wt = 0;
        if (wt >= NUM_WAVES) wt = NUM_WAVES - 1;
        return WAVE_NAMES[wt];
    }
    snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}
