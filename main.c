#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bfxr_params.h"
#include "bfxr_generator.h"
#include "bfxr_wav.h"
#include "bfxr_file.h"

void print_usage(const char* prog) {
    printf("Usage:\n");
    printf("  %s play <preset.bfxr>     - Play a preset (requires raylib)\n", prog);
    printf("  %s export <preset.bfxr> <out.wav> - Export to WAV\n", prog);
    printf("  %s random <out.wav>        - Generate random sound and export\n", prog);
    printf("  %s blend <a.bfxr> <b.bfxr> <t> <out.wav> - Blend two presets\n", prog);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "export") == 0 && argc >= 4) {
        double params[NUM_PARAMS];
        if (bfxr_load_preset(argv[2], params) != 0) {
            printf("Failed to load preset: %s\n", argv[2]);
            return 1;
        }
        BfxrWave wave = bfxr_generate_wave(params);
        if (bfxr_wav_save(argv[3], &wave) != 0) {
            printf("Failed to save WAV: %s\n", argv[3]);
            bfxr_wave_free(&wave);
            return 1;
        }
        printf("Exported to %s (%d samples)\n", argv[3], wave.num_samples);
        bfxr_wave_free(&wave);
        return 0;
    }

    if (strcmp(argv[1], "random") == 0 && argc >= 3) {
        double params[NUM_PARAMS];
        params_randomize(params);
        BfxrWave wave = bfxr_generate_wave(params);
        if (bfxr_wav_save(argv[2], &wave) != 0) {
            printf("Failed to save WAV: %s\n", argv[2]);
            bfxr_wave_free(&wave);
            return 1;
        }
        printf("Random sound exported to %s\n", argv[2]);
        bfxr_wave_free(&wave);
        return 0;
    }

    if (strcmp(argv[1], "blend") == 0 && argc >= 6) {
        double params_a[NUM_PARAMS], params_b[NUM_PARAMS];
        if (bfxr_load_preset(argv[2], params_a) != 0 || bfxr_load_preset(argv[3], params_b) != 0) {
            printf("Failed to load presets\n");
            return 1;
        }
        double t = atof(argv[4]);
        double blended[NUM_PARAMS];
        params_blend(params_a, params_b, t, blended);
        BfxrWave wave = bfxr_generate_wave(blended);
        if (bfxr_wav_save(argv[5], &wave) != 0) {
            printf("Failed to save WAV\n");
            bfxr_wave_free(&wave);
            return 1;
        }
        printf("Blended sound exported to %s\n", argv[5]);
        bfxr_wave_free(&wave);
        return 0;
    }

    print_usage(argv[0]);
    return 1;
}
