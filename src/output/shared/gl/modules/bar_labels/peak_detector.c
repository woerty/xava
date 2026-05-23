#include "peak_detector.h"
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
