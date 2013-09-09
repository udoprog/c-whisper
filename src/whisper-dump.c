#include "wsp.h"

#include <string.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    wsp_error_t e;
    WSP_ERROR_INIT(&e);

    if (argc < 2) {
        printf("Usage: whisper-dump <file> [<start-timestamp> <end-timestamp>]\n");
        return 1;
    }

    int with_time_interval = 0;
    wsp_time_t time_from = 0;
    wsp_time_t time_until = 0;

    if (argc == 4) {
        with_time_interval = 1;
        time_from = wsp_time_from_timestamp(atoi(argv[2]));
        time_until = wsp_time_from_timestamp(atoi(argv[3]));
    }

    const char *p = argv[1];
    wsp_t w;
    WSP_INIT(&w);

    if (wsp_open(&w, p, WSP_MMAP, WSP_READ, &e) == WSP_ERROR) {
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

        uint32_t count;

        if (with_time_interval) {
            if (wsp_fetch_time_points(&w, archive, time_from, time_until, points, &count, &e) == WSP_ERROR) {
                printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), p);
                return 1;
            }
        }
        else {
            count = archive->count;

            if (wsp_load_points(&w, archive, 0, archive->count, points, &e) == WSP_ERROR) {
                printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), p);
                return 1;
            }
        }

        printf("Archive #%u data:\n", i);

        for (j = 0; j < count; j++) {
            point = points[j];
            printf("%u: %u, %.8f\n", j, point.timestamp, point.value);
        }

        printf("\n");
    }

    wsp_close(&w, &e);

    return 0;
}
