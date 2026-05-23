#include "../note_naming.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
    char buf[16];

    freq_to_note_name(440.0, buf, sizeof(buf));
    assert(strcmp(buf, "A4") == 0);

    freq_to_note_name(261.63, buf, sizeof(buf));
    assert(strcmp(buf, "C4") == 0);

    freq_to_note_name(27.5, buf, sizeof(buf));
    assert(strcmp(buf, "A0") == 0);

    freq_to_note_name(4186.01, buf, sizeof(buf));
    assert(strcmp(buf, "C8") == 0);

    freq_to_note_name(445.0, buf, sizeof(buf));
    assert(strcmp(buf, "A4") == 0);

    freq_to_note_name(0.0, buf, sizeof(buf));
    assert(strcmp(buf, "?") == 0);

    freq_to_note_name(-100.0, buf, sizeof(buf));
    assert(strcmp(buf, "?") == 0);

    printf("test_note_naming: OK\n");
    return 0;
}
