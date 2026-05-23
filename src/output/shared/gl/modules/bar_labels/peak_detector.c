#include "peak_detector.h"
#include <stdbool.h>
#include <stdlib.h>

static int cmp_peak_desc(const void *a, const void *b) {
    uint32_t ha = ((const peak_t *)a)->height;
    uint32_t hb = ((const peak_t *)b)->height;
    if (ha < hb) return 1;
    if (ha > hb) return -1;
    return 0;
}

int peak_detector_find(
    const uint32_t *bars, uint32_t n,
    double min_height_rel,
    int max_peaks,
    peak_t *out
) {
    if (n < 3 || max_peaks <= 0) return 0;

    uint32_t max_h = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (bars[i] > max_h) max_h = bars[i];
    }
    if (max_h == 0) return 0;

    double threshold = (double)max_h * min_height_rel;

    peak_t *scratch = malloc(sizeof(peak_t) * (n - 2));
    int count = 0;
    for (uint32_t i = 1; i < n - 1; i++) {
        if (bars[i] > bars[i-1] && bars[i] > bars[i+1] && (double)bars[i] >= threshold) {
            scratch[count].bin = i;
            scratch[count].height = bars[i];
            count++;
        }
    }

    qsort(scratch, count, sizeof(peak_t), cmp_peak_desc);
    int keep = count < max_peaks ? count : max_peaks;
    for (int j = 0; j < keep; j++) out[j] = scratch[j];

    free(scratch);
    return keep;
}

int peak_detector_find_sticky(
    const uint32_t *bars, uint32_t n,
    double min_height_rel,
    int max_peaks,
    peak_t *last, int *last_age,
    int last_count,
    int hysteresis_frames
) {
    peak_t fresh[16];
    int fn = peak_detector_find(bars, n, min_height_rel,
        max_peaks > 16 ? 16 : max_peaks, fresh);

    if (hysteresis_frames <= 0 || last_count == 0) {
        for (int i = 0; i < fn; i++) {
            last[i] = fresh[i];
            last_age[i] = 0;
        }
        return fn;
    }

    /* One-to-one matching between sticky slots and fresh peaks:
     *
     * Each fresh peak can claim AT MOST one sticky slot (within +/-1 bin).
     * Matched sticky stays pinned at its original bin (no horizontal jitter),
     * only its age resets. Unmatched sticky ages out after hysteresis_frames.
     * Unclaimed fresh peaks are appended as new slots.
     *
     * This prevents two failure modes:
     *  - All slots collapsing to the loudest peak (old code kicked every
     *    slot to fresh[0] in parallel)
     *  - Sticky bin wobbling between adjacent bins frame-to-frame */
    bool fresh_claimed[16] = {false};
    int kept = 0;

    for (int i = 0; i < last_count; i++) {
        int match = -1;
        for (int j = 0; j < fn; j++) {
            if (fresh_claimed[j]) continue;
            int dbin = (int)fresh[j].bin - (int)last[i].bin;
            if (dbin >= -1 && dbin <= 1) { match = j; break; }
        }
        if (match >= 0) {
            /* Region still active — keep sticky bin/height, refresh age */
            if (kept != i) last[kept] = last[i];
            last_age[kept] = 0;
            fresh_claimed[match] = true;
            kept++;
        } else {
            /* No nearby fresh — age, drop if expired */
            int new_age = last_age[i] + 1;
            if (new_age <= hysteresis_frames) {
                if (kept != i) last[kept] = last[i];
                last_age[kept] = new_age;
                kept++;
            }
        }
    }

    /* Append fresh peaks that no sticky claimed — these are new spectral
     * features and deserve their own label slot. */
    int out_n = kept;
    for (int j = 0; j < fn && out_n < max_peaks; j++) {
        if (fresh_claimed[j]) continue;
        last[out_n] = fresh[j];
        last_age[out_n] = 0;
        out_n++;
    }
    return out_n;
}
