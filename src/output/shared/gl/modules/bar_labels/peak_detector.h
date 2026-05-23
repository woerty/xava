#ifndef XAVA_BAR_LABELS_PEAK_DETECTOR_H
#define XAVA_BAR_LABELS_PEAK_DETECTOR_H

#include <stdint.h>

typedef struct {
    uint32_t bin;
    uint32_t height;
} peak_t;

// Find local maxima in bars[] that exceed min_height_rel * max(bars).
// Edge bins (0, n-1) are ignored. Plateaus: strict > rule, so adjacent equal
// heights yield no peak. If more candidates than max_peaks, top-N by height
// are kept. Returns number of peaks written to out (<= max_peaks).
int peak_detector_find(
    const uint32_t *bars, uint32_t n,
    double min_height_rel,
    int max_peaks,
    peak_t *out
);

/* Sticky version: previous frame's peaks survive if no current candidate
 * exceeds them by 20% within `hysteresis_frames` frames. Caller owns
 * `last[]` and `last_age[]` across calls (size `max_peaks`).
 * `hysteresis_frames <= 0` disables stickiness. */
int peak_detector_find_sticky(
    const uint32_t *bars, uint32_t n,
    double min_height_rel,
    int max_peaks,
    peak_t *last, int *last_age,
    int last_count,
    int hysteresis_frames
);

#endif
