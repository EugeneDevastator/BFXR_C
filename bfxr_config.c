#include "bfxr_config.h"
#include "ini.h"
#include "bfxr_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE "bfxr.cfg"
#define SCENE_FILE "lastscene.bfxr"

static int config_handler(void* user, const char* section, const char* name, const char* value) {
    BfxrConfig* cfg = (BfxrConfig*)user;

    if (strcmp(section, "Audio") == 0) {
        if (strcmp(name, "volume") == 0) cfg->volume = (float)atof(value);
        else if (strcmp(name, "autoplay") == 0) cfg->autoplay = atoi(value);
    } else if (strcmp(section, "Spectrogram") == 0) {
        for (int i = 0; i < 3; i++) {
            char buf[32];
            snprintf(buf, sizeof(buf), "grad_t%d", i);
            if (strcmp(name, buf) == 0) cfg->grad_t[i] = (float)atof(value);
            snprintf(buf, sizeof(buf), "grad_r%d", i);
            if (strcmp(name, buf) == 0) cfg->grad_r[i] = (float)atof(value);
            snprintf(buf, sizeof(buf), "grad_g%d", i);
            if (strcmp(name, buf) == 0) cfg->grad_g[i] = (float)atof(value);
            snprintf(buf, sizeof(buf), "grad_b%d", i);
            if (strcmp(name, buf) == 0) cfg->grad_b[i] = (float)atof(value);
        }
    }
    return 1;
}

void config_load(BfxrConfig* cfg) {
    cfg->volume = 1.0f;
    cfg->autoplay = 1;
    cfg->grad_t[0] = 0.0f; cfg->grad_r[0] = 0; cfg->grad_g[0] = 0; cfg->grad_b[0] = 0;
    cfg->grad_t[1] = 0.6f; cfg->grad_r[1] = 32; cfg->grad_g[1] = 64; cfg->grad_b[1] = 128;
    cfg->grad_t[2] = 1.0f; cfg->grad_r[2] = 255; cfg->grad_g[2] = 255; cfg->grad_b[2] = 255;

    FILE* f = fopen(CONFIG_FILE, "r");
    if (f) {
        fclose(f);
        ini_parse(CONFIG_FILE, config_handler, cfg);
    }
}

void config_save(const BfxrConfig* cfg) {
    FILE* f = fopen(CONFIG_FILE, "w");
    if (!f) return;

    fprintf(f, "[Audio]\n");
    fprintf(f, "volume = %.2f\n", cfg->volume);
    fprintf(f, "autoplay = %d\n\n", cfg->autoplay);

    fprintf(f, "[Spectrogram]\n");
    for (int i = 0; i < 3; i++) {
        fprintf(f, "grad_t%d = %.1f\n", i, cfg->grad_t[i]);
        fprintf(f, "grad_r%d = %.0f\n", i, cfg->grad_r[i]);
        fprintf(f, "grad_g%d = %.0f\n", i, cfg->grad_g[i]);
        fprintf(f, "grad_b%d = %.0f\n", i, cfg->grad_b[i]);
    }

    fclose(f);
}

void config_load_scene(double params_l[NUM_PARAMS], double params_r[NUM_PARAMS], double* blend_t) {
    params_make_default(params_l);
    params_make_default(params_r);
    *blend_t = 0.5;
    FILE* f = fopen(SCENE_FILE, "r");
    if (f) {
        fclose(f);
        bfxr_load_scene(SCENE_FILE, params_l, params_r, blend_t);
    }
}

void config_save_scene(const double params_l[NUM_PARAMS], const double params_r[NUM_PARAMS], double blend_t) {
    bfxr_save_scene(SCENE_FILE, params_l, params_r, blend_t);
}
