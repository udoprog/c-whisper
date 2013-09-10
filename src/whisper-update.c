#include "wsp.h"

#include <string.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    if (argc < 3) {
        printf("Usage: whisper-update <file> <timestamp>:<value> ..\n");
        return 1;
    }

    const char *path = argv[1];

    int i;
    int c;
    uint32_t count = argc - 2;

    wsp_point_input_t points[count];

    for (i = 2, c = 0; c < count; i++, c++) {
        const char *input = argv[i];

        if (wsp_parse_point_input(input, points + c) == WSP_ERROR) {
            printf("Invalid archive specification: %s\n", input);
            return 1;
        }
    }

    wsp_t w;
    WSP_INIT(&w);

    if (wsp_open(&w, path, WSP_MMAP, WSP_READ | WSP_WRITE, &e) == WSP_ERROR) {
        printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), path);
        return 1;
    }

    for (i = 0; i < count; i++) {
        wsp_point_input_t *point = points + i;

        if (wsp_update(&w, point, &e) == WSP_ERROR) {
            printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), path);
            return 1;
        }
    }

    wsp_close(&w, &e);

    return 0;
}
