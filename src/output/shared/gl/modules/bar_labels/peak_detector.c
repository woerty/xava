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

    /* Track which fresh peaks were evaluated as kick candidates so they are
     * not also appended as extra peaks (avoids count inflation from adjacent
     * bins that lost the hysteresis check). */
    bool fresh_used[16] = {false};

    /* For each previous peak: if a fresh peak is >20% taller, adopt it;
     * otherwise age the sticky peak. Expired peaks get replaced by the
     * strongest fresh peak. */
    for (int i = 0; i < last_count; i++) {
        bool kicked = false;
        for (int j = 0; j < fn; j++) {
            fresh_used[j] = true;  /* evaluated against this sticky slot */
            if ((double)fresh[j].height > (double)last[i].height * 1.20) {
                last[i] = fresh[j];
                last_age[i] = 0;
                kicked = true;
                break;
            }
        }
        if (!kicked) {
            last_age[i]++;
            if (last_age[i] > hysteresis_frames && fn > 0) {
                last[i] = fresh[0];
                last_age[i] = 0;
            }
        }
    }

    /* Append fresh peaks that weren't evaluated as kick candidates and aren't
     * already in last[] (up to max_peaks). These represent genuinely new
     * spectral features beyond the existing sticky peaks. */
    int out_n = last_count;
    for (int j = 0; j < fn && out_n < max_peaks; j++) {
        if (fresh_used[j]) continue;
        bool already = false;
        for (int i = 0; i < out_n; i++) {
            if (last[i].bin == fresh[j].bin) { already = true; break; }
        }
        if (!already) {
            last[out_n] = fresh[j];
            last_age[out_n] = 0;
            out_n++;
        }
    }
    return out_n;
}
