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

/* Sticky version: one-to-one match between current and previous-frame peaks
 * (a fresh peak claims at most one sticky slot if its bin is within +/-1).
 * Claimed sticky slots reset their age and stay pinned at their original bin
 * to keep label positions stable. Unclaimed sticky slots increment age and
 * are dropped after hysteresis_frames without a match. Unclaimed fresh peaks
 * are appended as new slots (capped at max_peaks). Caller owns last[] and
 * last_age[] across calls. hysteresis_frames <= 0 disables stickiness. */
int peak_detector_find_sticky(
    const uint32_t *bars, uint32_t n,
    double min_height_rel,
    int max_peaks,
    peak_t *last, int *last_age,
    int last_count,
    int hysteresis_frames
);

#endif
