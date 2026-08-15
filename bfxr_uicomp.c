#include "bfxr_uicomp.h"
#include "bfxr_params.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Initialize components
void ui_slider_init(UiSlider* slider, int x, int y, int w, int h, double min, double max, const char* label) {
    slider->x = x;
    slider->y = y;
    slider->w = w;
    slider->h = h;
    slider->min = min;
    slider->max = max;
    slider->value = min;
    slider->label = label;
    slider->has_negative = (min < 0);
}

void ui_button_init(UiButton* btn, int x, int y, int w, int h, const char* label, Color color, int font_size) {
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->label = label;
    btn->color = color;
    btn->font_size = font_size;
    btn->hovered = 0;
    btn->clicked = 0;
}

void ui_blend_slider_init(UiBlendSlider* slider, int x, int y, int w, int h) {
    slider->x = x;
    slider->y = y;
    slider->w = w;
    slider->h = h;
    slider->value = 0.0;
}

void ui_volume_slider_init(UiVolumeSlider* slider, int x, int y, int w, int h) {
    slider->x = x;
    slider->y = y;
    slider->w = w;
    slider->h = h;
    slider->value = 1.0f;
}

// Draw parameter slider (used within panels)
void ui_draw_param_slider(int x, int y, int w, int h, int param_idx, double param_value, Font font, int lbl_font) {
    double t = params_to_t(param_idx, param_value);
    int bar_y = y + h / 2 - 4;

    DrawRectangle(x, bar_y, w, 8, (Color){200, 200, 200, 255});

    if (PARAM_RANGES[param_idx].min < 0) {
        int mid_x = x + w / 2;
        int fill_px = (int)(t * w) - w / 2;
        if (fill_px >= 0) {
            DrawRectangle(mid_x, bar_y, fill_px, 8, (Color){70, 130, 200, 255});
        } else {
            DrawRectangle(mid_x + fill_px, bar_y, -fill_px, 8, (Color){200, 100, 70, 255});
        }
        DrawLine(mid_x, bar_y - 2, mid_x, bar_y + 10, DARKGRAY);
    } else {
        int filled = (int)(t * w);
        if (filled > 0) {
            DrawRectangle(x, bar_y, filled, 8, (Color){70, 130, 200, 255});
        }
    }

    int knob_w = 6;
    int knob_h = 18;
    int knob_x = x + (int)(t * w) - knob_w / 2;
    DrawRectangle(knob_x, bar_y - (knob_h - 8) / 2, knob_w, knob_h, BLACK);

    const char* val_str = params_display(param_idx, param_value);
    DrawTextEx(font, val_str, (Vector2){x + w + 6, y}, lbl_font, 1, DARKGRAY);
}

// Handle parameter slider input
int ui_handle_param_slider_input(int mx, int my, int sx, int sw, int by, int row_h, double params[NUM_PARAMS]) {
    int released = 0;
    for (int i = 0; i < NUM_PARAMS; i++) {
        int track_y = by + i * row_h + row_h / 2;
        if (fabs((double)my - track_y) < 14 && mx >= sx && mx <= sx + sw) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                double t = (double)(mx - sx) / sw;
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;
                params[i] = t_to_param(i, t);
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                released = 1;
            }
        }
    }
    return released;
}

// Draw panel with parameter sliders
void ui_draw_param_panel(int x, int y, int w, int h, double params[NUM_PARAMS], const char* label,
                   int* out_sx, int* out_sw, int* out_by, int* out_row_h, Font font, int font_size, int slider_font_size) {
    DrawRectangleLines(x, y, w, h, LIGHTGRAY);
    DrawTextEx(font, label, (Vector2){x + 10, y + 8}, font_size, 1, DARKGRAY);

    int lbl_font = slider_font_size;
    int val_w = 72;
    int pad_left = x + 10;
    int row_h = (h - 50) / NUM_PARAMS;
    int by = y + 46;

    int max_lbl_w = 0;
    for (int i = 0; i < NUM_PARAMS; i++) {
        int tw = MeasureTextEx(font, PARAM_NAMES[i], lbl_font, 1).x;
        if (tw > max_lbl_w) max_lbl_w = tw;
    }
    int lbl_gap = 6;
    int sx = pad_left + max_lbl_w + lbl_gap;
    int sw = (x + w) - sx - val_w - 14;

    for (int i = 0; i < NUM_PARAMS; i++) {
        int sy = by + i * row_h;
        DrawTextEx(font, PARAM_NAMES[i], (Vector2){pad_left, sy}, lbl_font, 1, DARKGRAY);
        ui_draw_param_slider(sx, sy, sw, row_h, i, params[i], font, lbl_font);
    }

    if (out_sx) *out_sx = sx;
    if (out_sw) *out_sw = sw;
    if (out_by) *out_by = by;
    if (out_row_h) *out_row_h = row_h;
}

// Blend slider
void ui_blend_slider_draw(UiBlendSlider* slider, Font font, int font_size, int* released) {
    int mx = GetMouseX();
    int my = GetMouseY();

    int seg_w = slider->w / 3;
    int total_w = seg_w * 3;

    Color COLOR_EXTRA_A = {180, 100, 40, 180};
    Color COLOR_CORE = {200, 160, 40, 255};
    Color COLOR_EXTRA_B = {100, 180, 40, 180};
    Color COLOR_TRACK = {200, 200, 200, 255};

    int bar_h = 16;
    int bar_y = slider->y + slider->h / 2 - bar_h / 2;

    DrawRectangle(slider->x, bar_y, total_w, bar_h, COLOR_TRACK);

    int a_px = slider->x + seg_w;
    int b_px = slider->x + 2 * seg_w;
    int knob_px = slider->x + (int)((slider->value + 1.0) / 3.0 * total_w);

    if (slider->value < 0.0) {
        DrawRectangle(knob_px, bar_y, a_px - knob_px, bar_h, COLOR_EXTRA_A);
    } else if (slider->value <= 1.0) {
        DrawRectangle(a_px, bar_y, knob_px - a_px, bar_h, COLOR_CORE);
    } else {
        DrawRectangle(a_px, bar_y, b_px - a_px, bar_h, COLOR_CORE);
        DrawRectangle(b_px, bar_y, knob_px - b_px, bar_h, COLOR_EXTRA_B);
    }

    DrawLine(a_px, bar_y - 8, a_px, bar_y + bar_h + 8, (Color){40, 80, 160, 255});
    DrawLine(b_px, bar_y - 8, b_px, bar_y + bar_h + 8, (Color){40, 130, 60, 255});

    int lbl_fs = font_size - 4;
    DrawTextEx(font, "A", (Vector2){a_px - lbl_fs / 2 - 20, bar_y - lbl_fs / 2}, lbl_fs, 1, (Color){40, 80, 160, 255});
    DrawTextEx(font, "B", (Vector2){b_px - lbl_fs / 2 + 10, bar_y - lbl_fs / 2}, lbl_fs, 1, (Color){40, 130, 60, 255});

    int knob_w2 = 10;
    DrawRectangle(knob_px - knob_w2 / 2, bar_y - 6, knob_w2, bar_h + 12, BLACK);
    DrawTextEx(font, TextFormat("%.2f", slider->value), (Vector2){knob_px + 10, bar_y - lbl_fs / 2}, lbl_fs, 1, DARKGRAY);

    int touching = (abs(my - (bar_y + bar_h / 2)) < 24) && mx >= slider->x && mx <= slider->x + total_w;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && touching) {
        slider->value = (double)(mx - slider->x) / total_w * 3.0 - 1.0;
        if (slider->value < -1.0) slider->value = -1.0;
        if (slider->value > 2.0) slider->value = 2.0;
    }

    if (released) {
        *released = touching && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    }
}

int ui_blend_slider_handle_input(UiBlendSlider* slider, int mx, int my) {
    int bar_h = 16;
    int bar_y = slider->y + slider->h / 2 - bar_h / 2;
    int total_w = slider->w;
    int touching = (abs(my - (bar_y + bar_h / 2)) < 24) && mx >= slider->x && mx <= slider->x + total_w;

    if (touching && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        slider->value = (double)(mx - slider->x) / total_w * 3.0 - 1.0;
        if (slider->value < -1.0) slider->value = -1.0;
        if (slider->value > 2.0) slider->value = 2.0;
    }

    return touching && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

// Volume slider
void ui_volume_slider_draw(UiVolumeSlider* slider, Font font, int font_size) {
    int gap = 8;
    Vector2 lbl_w = MeasureTextEx(font, "Vol", font_size, 1);
    DrawTextEx(font, "Vol", (Vector2){slider->x - lbl_w.x - gap, slider->y}, font_size, 1, DARKGRAY);
    DrawRectangle(slider->x, slider->y, slider->w, 10, (Color){200, 200, 200, 255});
    int vol_filled = (int)(slider->value * slider->w);
    if (vol_filled > 0) {
        DrawRectangle(slider->x, slider->y, vol_filled, 10, (Color){200, 160, 40, 255});
    }
    DrawTextEx(font, TextFormat("%.2f", slider->value), (Vector2){slider->x + slider->w + gap, slider->y - 4}, font_size - 4, 1, DARKGRAY);
}

int ui_volume_slider_handle_input(UiVolumeSlider* slider, int mx, int my) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (abs(my - (slider->y + 5)) < 20 && mx >= slider->x && mx <= slider->x + slider->w) {
            slider->value = (float)((double)(mx - slider->x) / slider->w);
            if (slider->value < 0.0f) slider->value = 0.0f;
            if (slider->value > 1.0f) slider->value = 1.0f;
            return 1;
        }
    }
    return 0;
}

// Button
void ui_button_draw(UiButton* btn, Font font) {
    int mx = GetMouseX();
    int my = GetMouseY();

    btn->hovered = (mx >= btn->x && mx <= btn->x + btn->w && my >= btn->y && my <= btn->y + btn->h);
    Color btn_color = btn->hovered ? (Color){180, 180, 180, 255} : btn->color;

    DrawRectangle(btn->x, btn->y, btn->w, btn->h, btn_color);
    DrawRectangleLinesEx((Rectangle){btn->x, btn->y, btn->w, btn->h}, 1, DARKGRAY);
    int tw = MeasureTextEx(font, btn->label, btn->font_size, 1).x;
    DrawTextEx(font, btn->label, (Vector2){btn->x + (btn->w - tw) / 2, btn->y + 13}, btn->font_size, 1, RAYWHITE);
}

int ui_button_handle_input(UiButton* btn, int mx, int my) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (mx >= btn->x && mx <= btn->x + btn->w && my >= btn->y && my <= btn->y + btn->h) {
            btn->clicked = 1;
            return 1;
        }
    }
    btn->clicked = 0;
    return 0;
}
