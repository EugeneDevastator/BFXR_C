#ifndef BFXR_UI_H
#define BFXR_UI_H

#include "bfxr_params.h"
#include "raylib.h"

#define UI_FONT_SIZE 20
#define UI_SLIDER_FONT_SIZE 14
#define UI_PANEL_X 300
#define UI_PANEL_Y 50
#define UI_PANEL_W 680
#define UI_PANEL_H 650

void ui_draw_panel(int x, int y, int w, int h, const char* label);
void ui_draw_slider(int x, int y, int w, int h, double t, double min, double max,
                      const char* label, Color label_color, Color bar_color);
int ui_handle_slider_input(int mx, int my, int sx, int sw, int by, int row_h,
                            double* params, int* selected);

#endif
