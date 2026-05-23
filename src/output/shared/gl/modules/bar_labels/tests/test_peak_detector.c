#include "../peak_detector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_constant_array_no_peaks(void) {
    uint32_t bars[10] = {5,5,5,5,5,5,5,5,5,5};
    peak_t out[10];
    int n = peak_detector_find(bars, 10, 0.0, 10, out);
    assert(n == 0);
}

static void test_single_spike(void) {
    uint32_t bars[10] = {0,0,0,0,0,9,0,0,0,0};
    peak_t out[10];
    int n = peak_detector_find(bars, 10, 0.1, 10, out);
    assert(n == 1);
    assert(out[0].bin == 5);
    assert(out[0].height == 9);
}

static void test_multiple_spikes_topn(void) {
    uint32_t bars[10] = {0,3,0,7,0,5,0,9,0,1};
    peak_t out[2];
    int n = peak_detector_find(bars, 10, 0.0, 2, out);
    assert(n == 2);
    assert(out[0].height == 9);  // sorted desc
    assert(out[1].height == 7);
}

static void test_plateau_no_peak(void) {
    // Strict > rule: bars[1]=5 is not > bars[2]=5, so no peak.
    uint32_t bars[5] = {0,5,5,5,0};
    peak_t out[5];
    int n = peak_detector_find(bars, 5, 0.0, 5, out);
    assert(n == 0);
}

static void test_min_height_filter(void) {
    uint32_t bars[5] = {0,1,10,0,0};
    peak_t out[5];
    // max=10, threshold=0.9*10=9, bars[2]=10 >= 9 → qualifies, bars[1]=1 fails
    int n = peak_detector_find(bars, 5, 0.9, 5, out);
    assert(n == 1);
    assert(out[0].bin == 2);
}

static void test_edge_bins_ignored(void) {
    uint32_t bars[5] = {9,0,0,0,9};
    peak_t out[5];
    int n = peak_detector_find(bars, 5, 0.0, 5, out);
    assert(n == 0);
}

static void test_hysteresis_sticky(void) {
    peak_t last[3] = {0};
    int last_age[3] = {0};
    int last_n = 0;

    /* Frame 1: spike at bin 5, height 10 */
    uint32_t bars1[10] = {0,0,0,0,0,10,0,0,0,0};
    last_n = peak_detector_find_sticky(bars1, 10, 0.0, 3,
                                       last, last_age, last_n, 8);
    assert(last_n == 1 && last[0].bin == 5);

    /* Frame 2: spike moved to bin 6 (adjacent) — sticky stays pinned at bin 5 */
    uint32_t bars2[10] = {0,0,0,0,0,0,11,0,0,0};
    last_n = peak_detector_find_sticky(bars2, 10, 0.0, 3,
                                       last, last_age, last_n, 8);
    assert(last_n == 1 && last[0].bin == 5);

    /* Frame 3: even much louder bin 6 doesn't kick — same region absorbs it */
    uint32_t bars3[10] = {0,0,0,0,0,0,20,0,0,0};
    last_n = peak_detector_find_sticky(bars3, 10, 0.0, 3,
                                       last, last_age, last_n, 8);
    assert(last_n == 1 && last[0].bin == 5);

    /* Frame 4..12: peak disappears (bars=0), bin 5 ages out after 8 frames */
    uint32_t bars_zero[10] = {0};
    for (int f = 0; f < 9; f++) {
        last_n = peak_detector_find_sticky(bars_zero, 10, 0.0, 3,
                                           last, last_age, last_n, 8);
    }
    assert(last_n == 0);  /* expired */

    /* Frame 13: distant new peak at bin 2 — new slot */
    uint32_t bars_far[10] = {0,0,7,0,0,0,0,0,0,0};
    last_n = peak_detector_find_sticky(bars_far, 10, 0.0, 3,
                                       last, last_age, last_n, 8);
    assert(last_n == 1 && last[0].bin == 2);
}

static void test_two_distinct_peaks_stay_distinct(void) {
    /* Regression for the all-slots-collapse bug: with two well-separated
     * peaks, both sticky slots should remain pointing at their own bins
     * over multiple frames even when one is much louder than the other. */
    peak_t last[3] = {0};
    int last_age[3] = {0};
    int last_n = 0;

    uint32_t bars[10] = {0,0,5,0,0,0,0,30,0,0};  /* bin 2 (h=5), bin 7 (h=30) */
    last_n = peak_detector_find_sticky(bars, 10, 0.0, 3,
                                       last, last_age, last_n, 8);
    assert(last_n == 2);

    /* Two more frames with same spectrum — slots must stay distinct */
    for (int f = 0; f < 2; f++) {
        last_n = peak_detector_find_sticky(bars, 10, 0.0, 3,
                                           last, last_age, last_n, 8);
    }
    assert(last_n == 2);
    assert(last[0].bin != last[1].bin);  /* not collapsed */
}

static void test_hysteresis_disabled(void) {
    peak_t last[3] = {0};
    int last_age[3] = {0};
    int last_n = 0;

    uint32_t bars[10] = {0,0,0,0,0,10,0,0,0,0};
    last_n = peak_detector_find_sticky(bars, 10, 0.0, 3,
                                       last, last_age, last_n, 0);
    assert(last_n == 1 && last[0].bin == 5);
    /* With hysteresis_frames=0, second call should immediately adopt new */
    uint32_t bars2[10] = {0,0,0,0,0,0,7,0,0,0};
    last_n = peak_detector_find_sticky(bars2, 10, 0.0, 3,
                                       last, last_age, last_n, 0);
    assert(last_n == 1 && last[0].bin == 6);
}

int main(void) {
    test_constant_array_no_peaks();
    test_single_spike();
    test_multiple_spikes_topn();
    test_plateau_no_peak();
    test_min_height_filter();
    test_edge_bins_ignored();
    test_hysteresis_sticky();
    test_hysteresis_disabled();
    test_two_distinct_peaks_stay_distinct();
    printf("test_peak_detector: OK\n");
    return 0;
}
