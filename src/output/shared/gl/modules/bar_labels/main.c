#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "shared.h"
#include "output/shared/gl/main.h"
#include "output/shared/gl/util/shader.h"
#include "shared/log.h"
#include "label_renderer.h"
#include "peak_detector.h"
#include "note_naming.h"

/* ---- label mode enum ---- */
typedef enum {
    MODE_OFF  = 0,
    MODE_FREQ = 1,
    MODE_NOTE = 2,
    MODE_BOTH = 3,
} label_mode;

/* ---- module config ---- */
static struct bar_labels_cfg {
    label_mode  mode;
    double      min_height;
    int         max_peaks;
    char        font[512];
    int         font_size_px;
    uint32_t    color_rgb;
    int         hysteresis_frames;  /* reserved for Task 9 */
} cfg;

/* module program handle — needed only for its .config field */
static xava_gl_module_program pre;

static label_renderer *g_lr = NULL;

/* ---- hysteresis state ---- */
#define MAX_PEAKS_HARD 16
static peak_t   g_last_peaks[MAX_PEAKS_HARD];
static int      g_last_age[MAX_PEAKS_HARD];
static int      g_last_count = 0;

/* Per-slot smoothed bar-top height (pixels). Resets when the slot's
 * bin changes; otherwise low-passed against current xava->f[bin] to
 * keep label-y from jittering frame-to-frame. */
static float    g_smooth_h[MAX_PEAKS_HARD];
static uint32_t g_smooth_bin[MAX_PEAKS_HARD];
static int      g_smooth_valid[MAX_PEAKS_HARD];

/* ---- helpers ---- */
static label_mode parse_mode(const char *s) {
    if (!s)                              return MODE_NOTE;
    if (strcmp(s, "off")  == 0)         return MODE_OFF;
    if (strcmp(s, "freq") == 0)         return MODE_FREQ;
    if (strcmp(s, "note") == 0)         return MODE_NOTE;
    if (strcmp(s, "both") == 0)         return MODE_BOTH;
    xavaLog("bar_labels: unknown mode '%s', defaulting to 'note'", s);
    return MODE_NOTE;
}

/* ---- GL module callbacks ---- */

EXP_FUNC void xava_gl_module_ionotify_callback(xava_ionotify_event event,
        const char *filename, int id, XAVA *xava) {
    UNUSED(filename); UNUSED(id); UNUSED(xava); UNUSED(event);
}

EXP_FUNC void xava_gl_module_config_load(XAVAGLModule *module, XAVA *xava) {
    /* Load this module's config.ini (same pattern as bars/main.c) */
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_CONFIG, "", module, xava);

    /* mode */
    char *mode_str = xavaConfigGetString(pre.config, "bar_labels", "mode", "note");
    cfg.mode = parse_mode(mode_str);

    /* min_height */
    cfg.min_height = xavaConfigGetF64(pre.config, "bar_labels", "min_height", 0.5);

    /* max_peaks */
    cfg.max_peaks = xavaConfigGetI32(pre.config, "bar_labels", "max_peaks", 5);

    /* font */
    char *font_str = xavaConfigGetString(pre.config, "bar_labels",
                                         "font",
                                         "/usr/share/fonts/TTF/DejaVuSans.ttf");
    if (font_str) {
        strncpy(cfg.font, font_str, sizeof(cfg.font) - 1);
        cfg.font[sizeof(cfg.font) - 1] = '\0';
    } else {
        strncpy(cfg.font, "/usr/share/fonts/TTF/DejaVuSans.ttf", sizeof(cfg.font) - 1);
        cfg.font[sizeof(cfg.font) - 1] = '\0';
    }

    /* font_size_px */
    cfg.font_size_px = xavaConfigGetI32(pre.config, "bar_labels", "font_size_px", 14);

    /* color: hex RGB string -> uint32_t */
    char *color_str = xavaConfigGetString(pre.config, "bar_labels", "color", "ffffff");
    cfg.color_rgb = (uint32_t)strtoul(color_str ? color_str : "ffffff", NULL, 16);

    /* hysteresis_frames — read now, used in Task 9 */
    cfg.hysteresis_frames = xavaConfigGetI32(pre.config, "bar_labels", "hysteresis_frames", 8);

    xavaLog("bar_labels: config loaded — mode=%d min_height=%.2f max_peaks=%d "
            "font='%s' font_size=%d color=#%06x hysteresis=%d",
            cfg.mode, cfg.min_height, cfg.max_peaks,
            cfg.font, cfg.font_size_px, cfg.color_rgb, cfg.hysteresis_frames);
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    UNUSED(options);
    g_lr = label_renderer_create(cfg.font, (float)cfg.font_size_px);
    if (!g_lr) { xavaLog("bar_labels: label_renderer init failed"); }
    else { xavaLog("bar_labels: label_renderer init OK"); }
}

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    UNUSED(options);
    g_last_count = 0;
    memset(g_last_age, 0, sizeof(g_last_age));
    memset(g_smooth_valid, 0, sizeof(g_smooth_valid));
    xavaLog("bar_labels: apply");
}

EXP_FUNC void xava_gl_module_event(XAVAGLModuleOptions *options) {
    UNUSED(options);
}

EXP_FUNC void xava_gl_module_clear(XAVAGLModuleOptions *options) {
    UNUSED(options);
}

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;
    if (!g_lr || cfg.mode == MODE_OFF) return;
    if (xava->bars < 3 || !xava->bar_freq_lo) return;

    int max_n = cfg.max_peaks > MAX_PEAKS_HARD ? MAX_PEAKS_HARD : cfg.max_peaks;
    int np = peak_detector_find_sticky(xava->f, xava->bars, cfg.min_height,
        max_n, g_last_peaks, g_last_age, g_last_count, cfg.hysteresis_frames);
    g_last_count = np;

    label_renderer_begin(g_lr);

    /* Skip labels that would overlap a previously drawn one this frame.
     * Approx label width = font_size_px * 4 (covers "A#4 / 1234 Hz" worst-case). */
    float drawn_x[MAX_PEAKS_HARD];
    int   drawn_n = 0;
    float min_dx = (float)cfg.font_size_px * 4.0f;

    for (int i = 0; i < np; i++) {
        uint32_t bin = g_last_peaks[i].bin;
        double f_lo = xava->bar_freq_lo[bin];
        double f_hi = xava->bar_freq_hi[bin];
        double hz = sqrt(f_lo * f_hi);

        char text[32];
        switch (cfg.mode) {
            case MODE_FREQ:
                if (hz < 100.0) snprintf(text, sizeof(text), "%.1f Hz", hz);
                else            snprintf(text, sizeof(text), "%.0f Hz", hz);
                break;
            case MODE_NOTE: {
                char note[8];
                freq_to_note_name(hz, note, sizeof(note));
                snprintf(text, sizeof(text), "%s", note);
                break;
            }
            case MODE_BOTH: {
                char note[8];
                freq_to_note_name(hz, note, sizeof(note));
                if (hz < 100.0) snprintf(text, sizeof(text), "%s / %.1f Hz", note, hz);
                else            snprintf(text, sizeof(text), "%s / %.0f Hz", note, hz);
                break;
            }
            default: continue;
        }

        /* Pixel-coord position: center of bar, above peak.
         * Bars are drawn bottom-up; bar top = inner.h - f[bin].
         * We label just above the bar top (y decreases upward in GL,
         * but label_renderer uses screen-space y=0 at top). */
        float bar_pixel_w = (float)xava->conf.bw + (float)xava->conf.bs;
        float x = (float)xava->rest
                  + (float)bin * bar_pixel_w
                  + (float)xava->conf.bw / 2.0f
                  - (float)cfg.font_size_px * 1.5f;
        /* Smooth bar-top height per slot to reduce vertical label jitter.
         * Reset when bin changes; otherwise EMA with alpha=0.15 (lag ~6 frames). */
        float cur_h = (float)xava->f[bin];
        if (!g_smooth_valid[i] || g_smooth_bin[i] != bin) {
            g_smooth_h[i]     = cur_h;
            g_smooth_bin[i]   = bin;
            g_smooth_valid[i] = 1;
        } else {
            g_smooth_h[i] = g_smooth_h[i] * 0.93f + cur_h * 0.07f;
        }
        float bar_top_px = (float)xava->inner.h - g_smooth_h[i];
        float y = bar_top_px - 4.0f;
        /* If label would clip off the top, push it below the bar top instead */
        if (y < (float)cfg.font_size_px)
            y = bar_top_px + (float)cfg.font_size_px + 4.0f;

        /* Account for inner offset within outer window */
        x += (float)xava->inner.x;
        y += (float)xava->inner.y;

        /* Skip labels that would overlap a previously drawn one this frame. */
        bool overlap = false;
        for (int k = 0; k < drawn_n; k++) {
            float dx = x - drawn_x[k];
            if (dx < 0) dx = -dx;
            if (dx < min_dx) { overlap = true; break; }
        }
        if (overlap) continue;
        drawn_x[drawn_n++] = x;

        label_renderer_draw_label(g_lr, text, x, y);
    }

    float r = ((cfg.color_rgb >> 16) & 0xff) / 255.0f;
    float g = ((cfg.color_rgb >>  8) & 0xff) / 255.0f;
    float b = ( cfg.color_rgb        & 0xff) / 255.0f;
    label_renderer_flush(g_lr, xava->outer.w, xava->outer.h, r, g, b, 1.0f);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    UNUSED(options);
    label_renderer_destroy(g_lr);
    g_lr = NULL;
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}
