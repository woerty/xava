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

int main(void) {
    test_constant_array_no_peaks();
    test_single_spike();
    test_multiple_spikes_topn();
    test_plateau_no_peak();
    test_min_height_filter();
    test_edge_bins_ignored();
    printf("test_peak_detector: OK\n");
    return 0;
}
