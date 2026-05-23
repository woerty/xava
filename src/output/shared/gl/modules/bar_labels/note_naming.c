#include "note_naming.h"
#include <math.h>
#include <stdio.h>

void freq_to_note_name(double hz, char *buf, size_t sz) {
    static const char *names[] = {
        "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
    };
    if (hz <= 0.0) { snprintf(buf, sz, "?"); return; }
    double midi = 12.0 * log2(hz / 440.0) + 69.0;
    int n = (int)lround(midi);
    int octave = n / 12 - 1;
    int pc = ((n % 12) + 12) % 12;
    snprintf(buf, sz, "%s%d", names[pc], octave);
}
