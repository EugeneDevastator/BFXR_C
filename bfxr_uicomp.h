#ifndef BFXR_UICOMP_H
#define BFXR_UICOMP_H

#include "raylib.h"
#include "bfxr_params.h"

#ifdef __cplusplus
extern "C" {
#endif

// Slider component
typedef struct {
    int x, y, w, h;
    double min, max;
    double value;
    const char* label;
    int has_negative;
} UiSlider;

// Button component
typedef struct {
    int x, y, w, h;
    const char* label;
    Color color;
    int font_size;
    int hovered;
    int clicked;
} UiButton;

// Blend slider component
typedef struct {
    int x, y, w, h;
    double value;
} UiBlendSlider;

// Volume slider component
typedef struct {
    int x, y, w, h;
    float value;
} UiVolumeSlider;

// Initialize components
void ui_slider_init(UiSlider* slider, int x, int y, int w, int h, double min, double max, const char* label);
void ui_button_init(UiButton* btn, int x, int y, int w, int h, const char* label, Color color, int font_size);
void ui_blend_slider_init(UiBlendSlider* slider, int x, int y, int w, int h);
void ui_volume_slider_init(UiVolumeSlider* slider, int x, int y, int w, int h);

// Draw functions
void ui_slider_draw(UiSlider* slider, Font font, int slider_font_size);
void ui_button_draw(UiButton* btn, Font font);
void ui_blend_slider_draw(UiBlendSlider* slider, Font font, int font_size, int* released);
void ui_volume_slider_draw(UiVolumeSlider* slider, Font font, int font_size);

// Input handling
int ui_slider_handle_input(UiSlider* slider, int mx, int my);
int ui_button_handle_input(UiButton* btn, int mx, int my);
int ui_blend_slider_handle_input(UiBlendSlider* slider, int mx, int my);
int ui_volume_slider_handle_input(UiVolumeSlider* slider, int mx, int my);

// Parameter panel slider functions
void ui_draw_param_slider(int x, int y, int w, int h, int param_idx, double param_value, Font font, int lbl_font);
int ui_handle_param_slider_input(int mx, int my, int sx, int sw, int by, int row_h, double params[NUM_PARAMS]);

// Panel drawing
void ui_draw_param_panel(int x, int y, int w, int h, double params[NUM_PARAMS], const char* label,
                   int* out_sx, int* out_sw, int* out_by, int* out_row_h, Font font, int font_size, int slider_font_size);

#ifdef __cplusplus
}
#endif

#endif
