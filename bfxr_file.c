#include "bfxr_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int bfxr_save_preset(const char* filename, const double params[NUM_PARAMS]) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    for (int i = 0; i < NUM_PARAMS; i++) {
        fprintf(fp, "%.6f\n", params[i]);
    }
    fclose(fp);
    return 0;
}

int bfxr_load_preset(const char* filename, double params[NUM_PARAMS]) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    for (int i = 0; i < NUM_PARAMS; i++) {
        if (fscanf(fp, "%lf", &params[i]) != 1) {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

int bfxr_save_scene(const char* filename, const double params_l[NUM_PARAMS], const double params_r[NUM_PARAMS], double blend_t) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    for (int i = 0; i < NUM_PARAMS; i++) {
        fprintf(fp, "%.6f\n", params_l[i]);
    }
    for (int i = 0; i < NUM_PARAMS; i++) {
        fprintf(fp, "%.6f\n", params_r[i]);
    }
    fprintf(fp, "%.6f\n", blend_t);
    fclose(fp);
    return 0;
}

int bfxr_load_scene(const char* filename, double params_l[NUM_PARAMS], double params_r[NUM_PARAMS], double* blend_t) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    for (int i = 0; i < NUM_PARAMS; i++) {
        if (fscanf(fp, "%lf", &params_l[i]) != 1) {
            fclose(fp);
            return -1;
        }
    }
    for (int i = 0; i < NUM_PARAMS; i++) {
        if (fscanf(fp, "%lf", &params_r[i]) != 1) {
            fclose(fp);
            return -1;
        }
    }
    if (fscanf(fp, "%lf", blend_t) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}
