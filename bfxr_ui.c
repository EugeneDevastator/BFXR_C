#include "bfxr_ui.h"
#include <math.h>

void ui_draw_panel(int x, int y, int w, int h, const char* label) {
    DrawRectangleLines(x, y, w, h, LIGHTGRAY);
    DrawText(label, x + 10, y + 8, UI_FONT_SIZE, DARKGRAY);
}

void ui_draw_slider(int sx, int sy, int sw, int sh, double t, double min, double max,
                      const char* label, Color label_color, Color bar_color) {
    (void)min; (void)max;  // Suppress unused parameter warnings
    int track_y = sy + sh / 2;
    int bar_h = 8;
    int bar_y = track_y - bar_h / 2;
    
    DrawRectangle(sx, bar_y, sw, bar_h, (Color){200, 200, 200, 255});
    
    int filled = (int)(t * sw);
    if (filled > 0) {
        DrawRectangle(sx, bar_y, filled, bar_h, bar_color);
    }
    
    int knob_w = 6;
    int knob_h = 18;
    int knob_x = sx + (int)(t * sw) - knob_w / 2;
    DrawRectangle(knob_x, bar_y - (knob_h - bar_h) / 2, knob_w, knob_h, BLACK);
    
    DrawText(label, sx - 150, sy, UI_SLIDER_FONT_SIZE, label_color);
}

int ui_handle_slider_input(int mx, int my, int sx, int sw, int by, int row_h, 
                            double* params, int* selected) {
    int released = 0;
    for (int i = 0; i < NUM_PARAMS; i++) {
        int track_y = by + i * row_h + row_h / 2;
        if (fabs((double)my - track_y) < 14 && mx >= sx && mx <= sx + sw) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                double t = (double)(mx - sx) / sw;
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;
                params[i] = t_to_param(i, t);
                *selected = i;
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                released = 1;
            }
        }
    }
    return released;
}
