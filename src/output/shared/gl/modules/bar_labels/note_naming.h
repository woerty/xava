#ifndef XAVA_BAR_LABELS_NOTE_NAMING_H
#define XAVA_BAR_LABELS_NOTE_NAMING_H

#include <stddef.h>

// Writes equal-tempered note name with octave (e.g. "A4", "C#3") for hz.
// A4 = 440 Hz. For hz <= 0, writes "?".
void freq_to_note_name(double hz, char *buf, size_t sz);

#endif
