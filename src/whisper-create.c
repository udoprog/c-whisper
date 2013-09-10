#include "wsp.h"

#include <string.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    if (argc < 3) {
        printf("Usage: whisper-create <file> <time-per-point>:<time-to-store> ..\n");
        return 1;
    }

    wsp_mapping_t mapping = WSP_MMAP;
    wsp_aggregation_t aggregation = WSP_AVERAGE;
    float x_files_factor = 0.5;

    const char *path = argv[1];

    int i;
    int c;
    uint32_t count = argc - 2;

    wsp_archive_input_t archives[count];

    for (i = 2, c = 0; c < count; i++, c++) {
        const char *input = argv[i];

        if (wsp_parse_archive_input(input, archives + c) == -1) {
            printf("Invalid archive specification: %s\n", input);
            return 1;
        }
    }

    if (wsp_create(path, archives, count, aggregation, x_files_factor, mapping, &e) == WSP_ERROR) {
        printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), path);
        return 1;
    }

    return 0;
}
