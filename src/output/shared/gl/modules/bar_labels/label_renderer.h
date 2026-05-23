#ifndef XAVA_BAR_LABELS_LABEL_RENDERER_H
#define XAVA_BAR_LABELS_LABEL_RENDERER_H

#include <GL/glew.h>
#include <stdbool.h>

typedef struct label_renderer label_renderer;

label_renderer *label_renderer_create(const char *ttf_path, float font_size_px);
void label_renderer_destroy(label_renderer *lr);

// Stubs for Task 7 — defined here so main.c can already call them harmlessly.
void label_renderer_begin(label_renderer *lr);
void label_renderer_draw_label(label_renderer *lr,
        const char *text,
        float pixel_x, float pixel_y);
void label_renderer_flush(label_renderer *lr,
        int screen_w, int screen_h,
        float r, float g, float b, float a);

#endif
