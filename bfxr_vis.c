#include "bfxr_vis.h"
#include "bfxr_fft.h"
#include "bfxr_params.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SPECTRO_W 256
#define SPECTRO_H 512
#define N_FFT 2048
#define OVERLAP 0.75f

static float grad_t[3] = {0.0f, 0.6f, 1.0f};
static float grad_r[3] = {0, 32, 255};
static float grad_g[3] = {0, 64, 255};
static float grad_b[3] = {0, 128, 255};

void vis_set_gradient(const float t[3], const float r[3], const float g[3], const float b[3]) {
    for (int i = 0; i < 3; i++) {
        grad_t[i] = t[i]; grad_r[i] = r[i]; grad_g[i] = g[i]; grad_b[i] = b[i];
    }
}

static Texture2D spectro_tex = {0};
static unsigned long long last_wave_hash = 0;

unsigned long long simple_hash(const void* data, int len) {
    unsigned long long hash = 5381;
    const unsigned char* p = (const unsigned char*)data;
    for (int i = 0; i < len; i++)
        hash = ((hash << 5) + hash) + p[i];
    return hash;
}

static void compute_spectrogram(const BfxrWave* wave, float* spectro, int spectro_w, int spectro_h) {
    int n = wave->num_samples;
    if (n == 0) return;

    int hop = (int)(N_FFT * (1.0f - OVERLAP));
    if (hop < 1) hop = 1;

    int num_bins_fft = N_FFT / 2 + 1;
    int num_frames = (n - N_FFT) / hop + 1;
    if (num_frames <= 0) return;

    float nyquist = SAMPLE_RATE * 0.5f;
    float log_min = log2f(20.0f);
    float log_max = log2f(nyquist);

    float* window = (float*)malloc(N_FFT * sizeof(float));
    for (int i = 0; i < N_FFT; i++)
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N_FFT - 1)));

    complex float* fft_buf = (complex float*)malloc(N_FFT * sizeof(complex float));
    float* power_db = (float*)malloc(num_frames * num_bins_fft * sizeof(float));
    float pmin = 1e9f, pmax = -1e9f;

    for (int frame = 0; frame < num_frames; frame++) {
        int start = frame * hop;
        for (int i = 0; i < N_FFT; i++) {
            fft_buf[i] = (start + i < n)
                ? (float)wave->samples[start + i] / 32767.0f * window[i]
                : 0;
        }
        bfxr_fft(fft_buf, N_FFT);
        for (int bin = 0; bin < num_bins_fft; bin++) {
            float real = crealf(fft_buf[bin]);
            float imag = cimagf(fft_buf[bin]);
            float mag = sqrtf(real * real + imag * imag);
            float db = 20.0f * log10f(mag + 1e-9f);
            power_db[frame * num_bins_fft + bin] = db;
            if (db < pmin) pmin = db;
            if (db > pmax) pmax = db;
        }
    }

    float range = pmax - pmin + 1e-9f;

    for (int frame = 0; frame < num_frames && frame < spectro_w; frame++) {
        for (int y = 0; y < spectro_h; y++) {
            float freq = powf(2.0f, log_min + (float)y / (spectro_h - 1) * (log_max - log_min));
            int fft_bin = (int)(freq / nyquist * (num_bins_fft - 1) + 0.5f);
            if (fft_bin < 0) fft_bin = 0;
            if (fft_bin >= num_bins_fft) fft_bin = num_bins_fft - 1;

            float v = (power_db[frame * num_bins_fft + fft_bin] - pmin) / range;
            if (v < 0) v = 0;
            if (v > 1) v = 1;
            spectro[frame * spectro_h + y] = v;
        }
    }

    free(power_db);
    free(fft_buf);
    free(window);
}

void vis_draw_waveform(const BfxrWave* wave, int x, int y, int w, int h) {
    if (!wave || wave->num_samples == 0) return;

    DrawRectangle(x, y, w, h, RAYWHITE);
    DrawRectangleLines(x, y, w, h, LIGHTGRAY);

    int n = wave->num_samples;
    int step = (n > w) ? n / w : 1;
    int cy = y + h / 2;

    Vector2 prev = {x, cy};
    for (int i = 0; i < n; i += step) {
        int px = x + (int)((float)i / n * w);
        int py = cy - (int)(wave->samples[i] / 32767.0f * (h / 2 - 2));
        if (i > 0)
            DrawLineV(prev, (Vector2){px, py}, BLACK);
        prev = (Vector2){px, py};
    }
}

void vis_draw_spectrogram_full(const BfxrWave* wave, int x, int y, int w, int h) {
    if (!wave || wave->num_samples == 0) return;

    unsigned long long hash = simple_hash(wave->samples, wave->num_samples * (int)sizeof(int16_t));

    if (hash != last_wave_hash || spectro_tex.id == 0) {
        last_wave_hash = hash;
        if (spectro_tex.id) UnloadTexture(spectro_tex);

        float* spectro = (float*)malloc(SPECTRO_W * SPECTRO_H * sizeof(float));
        if (!spectro) return;
        memset(spectro, 0, SPECTRO_W * SPECTRO_H * sizeof(float));
        compute_spectrogram(wave, spectro, SPECTRO_W, SPECTRO_H);

        unsigned char* rgba = (unsigned char*)malloc(SPECTRO_W * SPECTRO_H * 4);
        if (!rgba) { free(spectro); return; }

        for (int frame = 0; frame < SPECTRO_W; frame++) {
            for (int yy = 0; yy < SPECTRO_H; yy++) {
                float v = spectro[frame * SPECTRO_H + yy];

                int stop = 0;
                for (int s = 0; s < 2; s++) {
                    if (v >= grad_t[s + 1]) stop = s + 1;
                }

                float t = 0;
                if (stop < 2) {
                    float denom = grad_t[stop + 1] - grad_t[stop];
                    if (denom > 1e-9f)
                        t = (v - grad_t[stop]) / denom;
                }

                int row = (SPECTRO_H - 1 - yy);
                int idx = (row * SPECTRO_W + frame) * 4;
                rgba[idx + 0] = (unsigned char)(grad_r[stop] + t * (grad_r[stop + 1] - grad_r[stop]));
                rgba[idx + 1] = (unsigned char)(grad_g[stop] + t * (grad_g[stop + 1] - grad_g[stop]));
                rgba[idx + 2] = (unsigned char)(grad_b[stop] + t * (grad_b[stop + 1] - grad_b[stop]));
                rgba[idx + 3] = 255;
            }
        }

        Image img = {
            .data = rgba,
            .width = SPECTRO_W,
            .height = SPECTRO_H,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        spectro_tex = LoadTextureFromImage(img);

        free(rgba);
        free(spectro);
    }

    if (spectro_tex.id) {
        DrawTexturePro(spectro_tex,
            (Rectangle){0, 0, (float)spectro_tex.width, (float)spectro_tex.height},
            (Rectangle){x, y, (float)w, (float)h},
            (Vector2){0, 0}, 0.0f, WHITE);
    }
}

void vis_clear_cache(void) {
    if (spectro_tex.id) {
        UnloadTexture(spectro_tex);
        spectro_tex.id = 0;
    }
    last_wave_hash = 0;
}
