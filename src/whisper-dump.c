#include "wsp.h"

#include <string.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    if (argc < 2) {
        printf("Usage: whisper-dump <file>\n");
        return 1;
    }

    const char *p = argv[1];
    wsp_t w;
    WSP_INIT(&w);

    if (wsp_open(&w, p, WSP_MMAP, &e) == WSP_ERROR) {
        printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), p);
        return 1;
    }

    uint32_t i, j;
    wsp_archive_t *archive;

    printf("Meta data:\n");
    printf("  aggregation_type = %u\n", w.meta.aggregation);
    printf("  max_retention = %u\n", w.meta.max_retention);
    printf("  xff = %f\n", w.meta.x_files_factor);
    printf("  archives_count = %u\n", w.meta.archives_count);
    printf("\n");

    wsp_point_t point;

    for (i = 0; i < w.archives_count; i++) {
        archive = w.archives + i;

        printf("Archive #%u info:\n", i);

        printf("  offset = %u\n", archive->offset);
        printf("  seconds_per_point = %u\n", archive->spp);
        printf("  points = %u\n", archive->count);
        printf("  points_size = %zu\n", archive->points_size);
        printf("\n");

        wsp_point_t points[archive->count];

        if (wsp_load_points(&w, archive, 0, archive->count, points, &e) == WSP_ERROR) {
            printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), p);
            return 1;
        }

        printf("Archive #%u data:\n", i);

        for (j = 0; j < archive->count; j++) {
            point = points[j];
            printf("%u: %u, %.4f\n", j, point.timestamp, point.value);
        }

        printf("\n");
    }

    wsp_close(&w, &e);

    return 0;
}
