#include "os_util.h"
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "bfxr_params.h"
#include "bfxr_generator.h"
#include "bfxr_wav.h"
#include "bfxr_file.h"
#include "bfxr_ui.h"
#include "bfxr_vis.h"
#include "bfxr_config.h"
#include "bfxr_uicomp.h"
#include "raylib.h"

// For tracking parameter changes
static double prev_params_l[NUM_PARAMS] = {0};
static double prev_params_r[NUM_PARAMS] = {0};

// Forward declaration for simple_hash
unsigned long long simple_hash(const void* data, int len);

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800
#define FONT_SIZE 32
#define SLIDER_FONT_SIZE (FONT_SIZE - 6)
#define UNIFIED_BTN_W 180
#define UNIFIED_BTN_H 44
#define BTN_GAP 6

static Font _font = {0};

// Match Python's GenJob class
typedef struct {
    BfxrWave result;
    int running;
    int done;
    char label[64];
    pthread_t thread;
    int should_stop;
    double params[NUM_PARAMS];
    int wave_type_a;
    int wave_type_b;
    double blend_t;
    int is_blend;
} GenJob;

static GenJob gen_job = {0};

// Clone params
void clone_params(const double src[NUM_PARAMS], double dst[NUM_PARAMS]) {
    memcpy(dst, src, sizeof(double) * NUM_PARAMS);
}

// Thread function for wave generation
static void* generate_thread(void* arg) {
    GenJob* job = (GenJob*)arg;
    job->running = 1;
    job->done = 0;

    if (job->is_blend) {
        job->result = bfxr_generate_wave_blended(job->params, job->wave_type_a, job->wave_type_b, job->blend_t);
    } else {
        job->result = bfxr_generate_wave(job->params);
    }

    job->done = 1;
    job->running = 0;
    return NULL;
}

void start_generation(GenJob* job, const double params[NUM_PARAMS], int which_unused, const char* label) {
    (void)which_unused;
    if (job->running) return;

    memcpy(job->params, params, sizeof(double) * NUM_PARAMS);
    job->is_blend = 0;
    strncpy(job->label, label, sizeof(job->label)-1);
    job->should_stop = 0;

    pthread_create(&job->thread, NULL, generate_thread, job);
    pthread_detach(job->thread);
}

void start_generation_blended(GenJob* job, const double blended[NUM_PARAMS], int wt_a, int wt_b, double blend_t, const char* label) {
    if (job->running) return;

    memcpy(job->params, blended, sizeof(double) * NUM_PARAMS);
    job->wave_type_a = wt_a;
    job->wave_type_b = wt_b;
    job->blend_t = blend_t;
    job->is_blend = 1;
    strncpy(job->label, label, sizeof(job->label)-1);
    job->should_stop = 0;

    pthread_create(&job->thread, NULL, generate_thread, job);
    pthread_detach(job->thread);
}

// Poll for completed generation
BfxrWave poll_generation(GenJob* job, const char** label) {
    if (job->done && job->result.num_samples > 0) {
        BfxrWave wave = job->result;
        *label = job->label;
        job->result.num_samples = 0;
        job->result.samples = NULL;
        job->done = 0;
        return wave;
    }
    *label = NULL;
    return (BfxrWave){0};
}

typedef struct {
    double params_l[NUM_PARAMS];
    double params_r[NUM_PARAMS];
    double blend_t;
    int sound_loaded;
    Sound sound;
    char status[256];
    BfxrWave last_wave;
    int wave_valid;
    int play_on_gen;
    int autoplay_blend;
    float global_volume;
    int rel_l;
    int rel_r;
    BfxrWave pending_wave;
    int pending_play;
    const char* pending_label;
    BfxrConfig config;
} AppState;

void load_and_play_wave(AppState* state, BfxrWave wave, const char* label) {
    if (state->sound_loaded) {
        UnloadSound(state->sound);
        state->sound_loaded = 0;
    }

    Wave w;
    w.data = wave.samples;
    w.frameCount = wave.num_samples;
    w.sampleRate = 44100;
    w.sampleSize = 16;
    w.channels = 1;

    state->sound = LoadSoundFromWave(w);
    state->sound_loaded = 1;
    SetSoundVolume(state->sound, state->global_volume);
    PlaySound(state->sound);

    if (state->wave_valid) {
        bfxr_wave_free(&state->last_wave);
    }
    state->last_wave = wave;
    state->wave_valid = 1;

    snprintf(state->status, sizeof(state->status), "Playing %s: %d samples", label, wave.num_samples);
}

int main(void) {
    os_mkdir("Export");

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "bfxrc");

    int monitor = GetCurrentMonitor();
    int sw = GetMonitorWidth(monitor);
    int sh = GetMonitorHeight(monitor);

    SetWindowPosition(0, 0);
    SetWindowSize(sw, sh);
    SetTargetFPS(60);
    InitAudioDevice();

    _font = LoadFontEx("Cadman_Bold.otf", FONT_SIZE * 2, 0, 0);
    if (_font.texture.id == 0) {
        _font = GetFontDefault();
    } else {
        SetTextureFilter(_font.texture, TEXTURE_FILTER_BILINEAR);
    }

    AppState state;
    params_make_default(state.params_l);
    params_make_default(state.params_r);
    state.params_r[6] = 0.5;
    state.blend_t = 0.5;
    state.sound_loaded = 0;
    state.sound = (Sound){0};
    state.wave_valid = 0;
    memset(&state.last_wave, 0, sizeof(BfxrWave));
    state.play_on_gen = 1;
    state.autoplay_blend = 0;
    state.global_volume = 1.0f;
    state.rel_l = 0;
    state.rel_r = 0;
    state.pending_wave = (BfxrWave){0};
    state.pending_play = 0;
    state.pending_label = NULL;

    config_load(&state.config);
    state.play_on_gen = state.config.autoplay;
    state.global_volume = state.config.volume;

    vis_set_gradient(state.config.grad_t, state.config.grad_r, state.config.grad_g, state.config.grad_b);
    config_load_scene(state.params_l, state.params_r, &state.blend_t);

    memcpy(prev_params_l, state.params_l, sizeof(double) * NUM_PARAMS);
    memcpy(prev_params_r, state.params_r, sizeof(double) * NUM_PARAMS);

    strcpy(state.status, "Ready");

    if (state.play_on_gen) {
        start_generation(&gen_job, state.params_l, 0, "A");
    }

    gen_job.running = 0;
    gen_job.done = 0;
    gen_job.result = (BfxrWave){0};

    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        int mx = GetMouseX();
        int my = GetMouseY();

        if (gen_job.done) {
            const char* label = NULL;
            BfxrWave wave = poll_generation(&gen_job, &label);
            if (wave.num_samples > 0) {
                load_and_play_wave(&state, wave, label);
            }
        }

        int PANEL_W = (sw / 3 - 10) > 300 ? (sw / 3 - 10) : 300;
        int PANEL_H = sh - 60;
        int PANEL_Y = 30;
        int LEFT_X = 10;
        int RIGHT_X = sw - PANEL_W - 10;
        int CENTER_X = LEFT_X + PANEL_W + 10;
        int CENTER_W = RIGHT_X - CENTER_X - 10;

        int cx = CENTER_X;
        int cw = CENTER_W;

        int col1_x = cx + 4;
        int col3_x = cx + cw - UNIFIED_BTN_W - 4;
        int col2_x = col1_x + UNIFIED_BTN_W + 8;

        int CTRL_H = 28;
        int ctrl_y = PANEL_Y + 8;
        int vol_y = ctrl_y + CTRL_H + 36;
        int status_y = vol_y + CTRL_H + 4;
        int BTN_START = status_y + CTRL_H + 8;

        int left_btn_count = 6;
        int right_btn_count = 5;
        int max_btn_count = (left_btn_count > right_btn_count) ? left_btn_count : right_btn_count;
        int buttons_end_y = BTN_START + max_btn_count * (UNIFIED_BTN_H + BTN_GAP);

        int blend_slider_y1 = buttons_end_y + BTN_GAP;
        int blend_h = 40;
        int scene_btn_y = blend_slider_y1 + blend_h + BTN_GAP;

        int sx_l, sw_l, by_l, row_h_l;
        int sx_r, sw_r, by_r, row_h_r;

        ui_draw_param_panel(LEFT_X, PANEL_Y, PANEL_W, PANEL_H, state.params_l, "PRESET A", &sx_l, &sw_l, &by_l, &row_h_l, _font, FONT_SIZE, SLIDER_FONT_SIZE);
        state.rel_l = ui_handle_param_slider_input(mx, my, sx_l, sw_l, by_l, row_h_l, state.params_l);

        ui_draw_param_panel(RIGHT_X, PANEL_Y, PANEL_W, PANEL_H, state.params_r, "PRESET B", &sx_r, &sw_r, &by_r, &row_h_r, _font, FONT_SIZE, SLIDER_FONT_SIZE);
        state.rel_r = ui_handle_param_slider_input(mx, my, sx_r, sw_r, by_r, row_h_r, state.params_r);

        if ((state.play_on_gen || state.autoplay_blend) && !gen_job.running) {
            int l_changed = 0, r_changed = 0;
            for (int i = 0; i < NUM_PARAMS; i++) {
                if (state.params_l[i] != prev_params_l[i]) l_changed = 1;
                if (state.params_r[i] != prev_params_r[i]) r_changed = 1;
            }
            if (state.autoplay_blend && (l_changed || r_changed)) {
                double blended[NUM_PARAMS];
                params_blend(state.params_l, state.params_r, state.blend_t, blended);
                int wt_dom = (state.blend_t <= 0.5) ? (int)state.params_l[0] : (int)state.params_r[0];
                blended[0] = (double)wt_dom;
                start_generation_blended(&gen_job, blended, (int)state.params_l[0], (int)state.params_r[0], state.blend_t, "BLEND");
                snprintf(state.status, sizeof(state.status), "Generating BLEND...");
                memcpy(prev_params_l, state.params_l, sizeof(double) * NUM_PARAMS);
                memcpy(prev_params_r, state.params_r, sizeof(double) * NUM_PARAMS);
            } else {
                if (l_changed && state.play_on_gen) {
                    start_generation(&gen_job, state.params_l, 0, "A");
                    memcpy(prev_params_l, state.params_l, sizeof(double) * NUM_PARAMS);
                    snprintf(state.status, sizeof(state.status), "Generating A...");
                }
                if (r_changed && state.play_on_gen) {
                    start_generation(&gen_job, state.params_r, 1, "B");
                    memcpy(prev_params_r, state.params_r, sizeof(double) * NUM_PARAMS);
                    snprintf(state.status, sizeof(state.status), "Generating B...");
                }
            }
        } else if (!state.play_on_gen && !state.autoplay_blend) {
            memcpy(prev_params_l, state.params_l, sizeof(double) * NUM_PARAMS);
            memcpy(prev_params_r, state.params_r, sizeof(double) * NUM_PARAMS);
        }

        double old_blend_t = state.blend_t;
        UiBlendSlider blend_slider;
        ui_blend_slider_init(&blend_slider, cx, blend_slider_y1, cw, blend_h);
        blend_slider.value = state.blend_t;
        int blend_released = 0;
        ui_blend_slider_draw(&blend_slider, _font, SLIDER_FONT_SIZE, &blend_released);
        state.blend_t = blend_slider.value;

        if (state.play_on_gen && state.blend_t != old_blend_t && !gen_job.running) {
            double blended[NUM_PARAMS];
            params_blend(state.params_l, state.params_r, state.blend_t, blended);
            int wt_dom = (state.blend_t <= 0.5) ? (int)state.params_l[0] : (int)state.params_r[0];
            blended[0] = (double)wt_dom;
            start_generation_blended(&gen_job, blended, (int)state.params_l[0], (int)state.params_r[0], state.blend_t, "BLEND");
            snprintf(state.status, sizeof(state.status), "Generating BLEND...");
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int cy;
            const char* col1_labels[] = {"PLAY A", "A < BLEND", "A < RND", "A < B", "NOVEL A", "EXPORT A"};

            cy = BTN_START;
            for (int i = 0; i < 6; i++) {
                UiButton btn;
                ui_button_init(&btn, col1_x, cy, UNIFIED_BTN_W, UNIFIED_BTN_H, col1_labels[i], (Color){0}, FONT_SIZE - 8);
                if (ui_button_handle_input(&btn, mx, my)) {
                    switch(i) {
                        case 0:
                            if (!gen_job.running) {
                                start_generation(&gen_job, state.params_l, 0, "A");
                                snprintf(state.status, sizeof(state.status), "Generating A...");
                            }
                            break;
                        case 1: {
                            double blended[NUM_PARAMS];
                            params_blend(state.params_l, state.params_r, state.blend_t, blended);
                            int wt_dom = (state.blend_t <= 0.5) ? (int)state.params_l[0] : (int)state.params_r[0];
                            blended[0] = (double)wt_dom;
                            clone_params(blended, state.params_l);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_l, 0, "A");
                                snprintf(state.status, sizeof(state.status), "Generating A...");
                            }
                            break;
                        }
                        case 2:
                            params_randomize(state.params_l);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_l, 0, "A");
                                snprintf(state.status, sizeof(state.status), "Generating A...");
                            }
                            break;
                        case 3:
                            clone_params(state.params_r, state.params_l);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_l, 0, "A");
                                snprintf(state.status, sizeof(state.status), "Generating A...");
                            }
                            break;
                        case 4:
                            params_randomize(state.params_l);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_l, 0, "A");
                                snprintf(state.status, sizeof(state.status), "Generating A...");
                            }
                            break;
                        case 5: {
                            unsigned long long h = simple_hash(state.last_wave.samples, state.last_wave.num_samples * 2);
                            char export_name[256];
                            time_t now = time(NULL);
                            struct tm *t = localtime(&now);
                            char date_str[32];
                            strftime(date_str, sizeof(date_str), "%y_%m_%d", t);
                            snprintf(export_name, sizeof(export_name), "Export/Sample_A_%s_%llx.wav", date_str, h);
                            bfxr_wav_save(export_name, &state.last_wave);
                            snprintf(state.status, sizeof(state.status), "Exported %s", export_name);
                            break;
                        }
                    }
                }
                cy += UNIFIED_BTN_H + BTN_GAP;
            }

            const char* col3_labels[] = {"PLAY B", "BLEND > B", "RND > B", "A > B", "EXPORT B"};

            cy = BTN_START;
            for (int i = 0; i < 5; i++) {
                UiButton btn;
                ui_button_init(&btn, col3_x, cy, UNIFIED_BTN_W, UNIFIED_BTN_H, col3_labels[i], (Color){0}, FONT_SIZE - 8);
                if (ui_button_handle_input(&btn, mx, my)) {
                    switch(i) {
                        case 0:
                            if (!gen_job.running) {
                                start_generation(&gen_job, state.params_r, 1, "B");
                                snprintf(state.status, sizeof(state.status), "Generating B...");
                            }
                            break;
                        case 1: {
                            double blended[NUM_PARAMS];
                            params_blend(state.params_l, state.params_r, state.blend_t, blended);
                            int wt_dom = (state.blend_t <= 0.5) ? (int)state.params_l[0] : (int)state.params_r[0];
                            blended[0] = (double)wt_dom;
                            clone_params(blended, state.params_r);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_r, 1, "B");
                                snprintf(state.status, sizeof(state.status), "Generating B...");
                            }
                            break;
                        }
                        case 2:
                            params_randomize(state.params_r);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_r, 1, "B");
                                snprintf(state.status, sizeof(state.status), "Generating B...");
                            }
                            break;
                        case 3:
                            clone_params(state.params_l, state.params_r);
                            if (state.play_on_gen && !gen_job.running) {
                                start_generation(&gen_job, state.params_r, 1, "B");
                                snprintf(state.status, sizeof(state.status), "Generating B...");
                            }
                            break;
                        case 4: {
                            unsigned long long h = simple_hash(state.last_wave.samples, state.last_wave.num_samples * 2);
                            char export_name[256];
                            time_t now = time(NULL);
                            struct tm *t = localtime(&now);
                            char date_str[32];
                            strftime(date_str, sizeof(date_str), "%y_%m_%d", t);
                            snprintf(export_name, sizeof(export_name), "Export/Sample_B_%s_%llx.wav", date_str, h);
                            bfxr_wav_save(export_name, &state.last_wave);
                            snprintf(state.status, sizeof(state.status), "Exported %s", export_name);
                            break;
                        }
                    }
                }
                cy += UNIFIED_BTN_H + BTN_GAP;
            }

            UiButton btn_blend;
            ui_button_init(&btn_blend, col2_x, BTN_START, UNIFIED_BTN_W, UNIFIED_BTN_H, "PLAY BLEND", (Color){0}, FONT_SIZE - 8);
            if (ui_button_handle_input(&btn_blend, mx, my)) {
                if (!gen_job.running) {
                    double blended[NUM_PARAMS];
                    params_blend(state.params_l, state.params_r, state.blend_t, blended);
                    start_generation_blended(&gen_job, blended, (int)state.params_l[0], (int)state.params_r[0], state.blend_t, "BLEND");
                    snprintf(state.status, sizeof(state.status), "Generating BLEND...");
                }
            }

            UiButton btn_save;
            ui_button_init(&btn_save, col2_x, scene_btn_y, UNIFIED_BTN_W, UNIFIED_BTN_H, "SAVE SCENE", (Color){0}, FONT_SIZE - 10);
            if (ui_button_handle_input(&btn_save, mx, my)) {
                bfxr_save_scene("scene.bfxr", state.params_l, state.params_r, state.blend_t);
                strcpy(state.status, "Saved scene.bfxr");
            }

            UiButton btn_load;
            ui_button_init(&btn_load, col2_x, scene_btn_y + UNIFIED_BTN_H + BTN_GAP, UNIFIED_BTN_W, UNIFIED_BTN_H, "LOAD SCENE", (Color){0}, FONT_SIZE - 10);
            if (ui_button_handle_input(&btn_load, mx, my)) {
                if (bfxr_load_scene("scene.bfxr", state.params_l, state.params_r, &state.blend_t) == 0) {
                    strcpy(state.status, "Loaded scene.bfxr");
                }
            }

            UiButton btn_export;
            ui_button_init(&btn_export, col2_x, scene_btn_y + 2*(UNIFIED_BTN_H + BTN_GAP), UNIFIED_BTN_W, UNIFIED_BTN_H, "EXPORT BLEND", (Color){0}, FONT_SIZE - 10);
            if (ui_button_handle_input(&btn_export, mx, my)) {
                double blended[NUM_PARAMS];
                params_blend(state.params_l, state.params_r, state.blend_t, blended);
                BfxrWave wave = bfxr_generate_wave_blended(blended, (int)state.params_l[0], (int)state.params_r[0], state.blend_t);
                unsigned long long h = simple_hash(wave.samples, wave.num_samples * 2);
                char export_name[256];
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char date_str[32];
                strftime(date_str, sizeof(date_str), "%y_%m_%d", t);
                snprintf(export_name, sizeof(export_name), "Export/Sample_BLEND_%s_%llx.wav", date_str, h);
                bfxr_wav_save(export_name, &wave);
                bfxr_wave_free(&wave);
                snprintf(state.status, sizeof(state.status), "Exported %s", export_name);
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (mx >= cx && mx <= cx + 26 && my >= ctrl_y && my <= ctrl_y + 26) {
                state.play_on_gen = !state.play_on_gen;
            }
            int autoplay_blend_y = ctrl_y + 30;
            if (mx >= cx && mx <= cx + 26 && my >= autoplay_blend_y && my <= autoplay_blend_y + 26) {
                state.autoplay_blend = !state.autoplay_blend;
            }
        }

        DrawRectangleLines(cx, ctrl_y + 30, 26, 26, DARKGRAY);
        if (state.autoplay_blend) {
            DrawRectangle(cx + 4, ctrl_y + 34, 18, 18, (Color){200, 120, 30, 255});
        }
        DrawTextEx(_font, "Autoplay Blend", (Vector2){cx + 34, ctrl_y + 34}, SLIDER_FONT_SIZE, 1, DARKGRAY);

        UiVolumeSlider vol_slider;
        {
            int gap = 8;
            Vector2 vol_lbl_w = MeasureTextEx(_font, "Vol", SLIDER_FONT_SIZE, 1);
            Vector2 vol_val_w = MeasureTextEx(_font, "1.00", SLIDER_FONT_SIZE - 4, 1);
            int vol_w = cw - (int)vol_lbl_w.x - (int)vol_val_w.x - gap * 3;
            if (vol_w < 60) vol_w = 60;
            int vol_total = (int)vol_lbl_w.x + gap + vol_w + gap + (int)vol_val_w.x;
            int vol_x = cx + (cw - vol_total) / 2 + (int)vol_lbl_w.x + gap;
            ui_volume_slider_init(&vol_slider, vol_x, vol_y, vol_w, 10);
        }
        vol_slider.value = state.global_volume;
        if (ui_volume_slider_handle_input(&vol_slider, mx, my)) {
            state.global_volume = vol_slider.value;
            if (state.sound_loaded) {
                SetSoundVolume(state.sound, state.global_volume);
            }
        }

        BeginDrawing();
        ClearBackground((Color){240, 240, 240, 255});

        ui_draw_param_panel(LEFT_X, PANEL_Y, PANEL_W, PANEL_H, state.params_l, "PRESET A", &sx_l, &sw_l, &by_l, &row_h_l, _font, FONT_SIZE, SLIDER_FONT_SIZE);
        ui_draw_param_panel(RIGHT_X, PANEL_Y, PANEL_W, PANEL_H, state.params_r, "PRESET B", &sx_r, &sw_r, &by_r, &row_h_r, _font, FONT_SIZE, SLIDER_FONT_SIZE);

        DrawRectangleLines(cx, ctrl_y, 26, 26, DARKGRAY);
        if (state.play_on_gen) {
            DrawRectangle(cx + 4, ctrl_y + 4, 18, 18, (Color){200, 120, 30, 255});
        }
        DrawTextEx(_font, "Play on Change", (Vector2){cx + 34, ctrl_y + 4}, SLIDER_FONT_SIZE, 1, DARKGRAY);

        ui_volume_slider_draw(&vol_slider, _font, SLIDER_FONT_SIZE);

        if (gen_job.running) {
            DrawTextEx(_font, "Generating...", (Vector2){cx, status_y}, SLIDER_FONT_SIZE, 1, (Color){180, 140, 0, 255});
        } else {
            DrawTextEx(_font, state.status, (Vector2){cx, status_y}, SLIDER_FONT_SIZE, 1, (Color){0, 100, 200, 255});
        }

        Color COLOR_A = {40, 80, 160, 255};
        Color COLOR_XFER = {200, 100, 70, 255};
        Color COLOR_RAND = {120, 60, 160, 255};
        Color COLOR_COPY = {80, 80, 80, 255};
        const char* col1_labels[] = {"PLAY A", "A < BLEND", "A < RND", "A < B", "NOVEL A", "EXPORT A"};
        Color col1_colors[] = {COLOR_A, COLOR_XFER, COLOR_RAND, COLOR_COPY, COLOR_RAND, {100, 100, 160, 255}};

        int cy = BTN_START;
        for (int i = 0; i < 6; i++) {
            UiButton btn;
            ui_button_init(&btn, col1_x, cy, UNIFIED_BTN_W, UNIFIED_BTN_H, col1_labels[i], col1_colors[i], FONT_SIZE - 8);
            ui_button_draw(&btn, _font);
            cy += UNIFIED_BTN_H + BTN_GAP;
        }

        Color COLOR_B = {40, 130, 60, 255};
        const char* col3_labels[] = {"PLAY B", "BLEND > B", "RND > B", "A > B", "EXPORT B"};
        Color col3_colors[] = {COLOR_B, COLOR_XFER, COLOR_RAND, COLOR_COPY, {100, 100, 160, 255}};

        cy = BTN_START;
        for (int i = 0; i < 5; i++) {
            UiButton btn;
            ui_button_init(&btn, col3_x, cy, UNIFIED_BTN_W, UNIFIED_BTN_H, col3_labels[i], col3_colors[i], FONT_SIZE - 8);
            ui_button_draw(&btn, _font);
            cy += UNIFIED_BTN_H + BTN_GAP;
        }

        UiButton btn_blend;
        ui_button_init(&btn_blend, col2_x, BTN_START, UNIFIED_BTN_W, UNIFIED_BTN_H, "PLAY BLEND", (Color){180, 130, 20, 255}, FONT_SIZE - 8);
        ui_button_draw(&btn_blend, _font);

        ui_blend_slider_draw(&blend_slider, _font, SLIDER_FONT_SIZE, &blend_released);

        UiButton btn_save;
        ui_button_init(&btn_save, col2_x, scene_btn_y, UNIFIED_BTN_W, UNIFIED_BTN_H, "SAVE SCENE", (Color){100, 100, 160, 255}, FONT_SIZE - 10);
        ui_button_draw(&btn_save, _font);

        UiButton btn_load;
        ui_button_init(&btn_load, col2_x, scene_btn_y + UNIFIED_BTN_H + BTN_GAP, UNIFIED_BTN_W, UNIFIED_BTN_H, "LOAD SCENE", (Color){100, 100, 160, 255}, FONT_SIZE - 10);
        ui_button_draw(&btn_load, _font);

        UiButton btn_export;
        ui_button_init(&btn_export, col2_x, scene_btn_y + 2*(UNIFIED_BTN_H + BTN_GAP), UNIFIED_BTN_W, UNIFIED_BTN_H, "EXPORT BLEND", (Color){100, 100, 160, 255}, FONT_SIZE - 10);
        ui_button_draw(&btn_export, _font);

        if (state.wave_valid) {
            int viz_y = sh - 380;
            int viz_w = CENTER_W - 20;
            int viz_h = 120;
            int spec_h = viz_h * 2;
            int viz_x = CENTER_X + 10;

            vis_draw_waveform(&state.last_wave, viz_x, viz_y, viz_w, viz_h);
            DrawTextEx(_font, "Waveform", (Vector2){viz_x, viz_y - 20}, 16, 1, DARKGRAY);

            vis_draw_spectrogram_full(&state.last_wave, viz_x, viz_y + viz_h + 10, viz_w, spec_h);
            DrawTextEx(_font, "Spectrogram", (Vector2){viz_x, viz_y + viz_h + 10 - 20}, 16, 1, DARKGRAY);
        }

        EndDrawing();
    }

    state.config.volume = state.global_volume;
    state.config.autoplay = state.play_on_gen;
    config_save(&state.config);
    config_save_scene(state.params_l, state.params_r, state.blend_t);

    if (state.sound_loaded) UnloadSound(state.sound);
    if (state.wave_valid) bfxr_wave_free(&state.last_wave);
    if (gen_job.result.num_samples > 0) bfxr_wave_free(&gen_job.result);
    vis_clear_cache();
    if (_font.texture.id) UnloadFont(_font);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
