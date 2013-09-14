// vim: foldmethod=marker

#include "wsp.h"
#include "wsp_private.h"
#include "wsp_time.h"

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "wsp_debug.h"

// static initialization {{{
const char *wsp_error_strings[WSP_ERROR_SIZE] = {
    /* WSP_ERROR_NONE */
    "No error",
    /* WSP_ERROR_IO */
    "I/O error",
    /* WSP_ERROR_NOT_OPEN */
    "Whisper file not open",
    /* WSP_ERROR_ALREADY_OPEN */
    "Whisper file already open",
    /* WSP_ERROR_MALLOC */
    "Allocation failure",
    /* WSP_ERROR_OFFSET */
    "Invalid offset",
    /* WSP_ERROR_FUTURE_TIMESTAMP */
    "Invalid future timestamp",
    /* WSP_ERROR_RETENTION */
    "Timestamp not covered by any archives in this database",
    /* WSP_ERROR_ARCHIVE */
    "Invalid data in archive",
    /* WSP_ERROR_POINT_OOB */
    "Point out of bounds",
    /* WSP_ERROR_UNKNOWN_AGGREGATION */
    "Unknown aggregation function",
    /* WSP_ERROR_ARCHIVE_MISALIGNED */
    "Archive headers are not aligned",
    /* WSP_ERROR_TIME_INTERVAL */
    "Invalid time interval",
    /* WSP_ERROR_IO_MODE */
    "Invalid open mode",
    /* WSP_ERROR_MMAP */
    "mmap failed",
    /* WSP_ERROR_FTRUNCATE */
    "ftruncate failed",
    /* WSP_ERROR_FSYNC */
    "fsync failed",
    /* WSP_ERROR_OPEN */
    "open failed",
    /* WSP_ERROR_FOPEN */
    "fopen failed",
    /* WSP_ERROR_FILENO */
    "fileno failed",
    /* WSP_ERROR_IO_MISSING */
    "Database not open for I/O",
    /* WSP_ERROR_IO_INVALID */
    "Invalid I/O operation for this instance",
    /* WSP_ERROR_IO_OFFSET */
    "I/O operations on invalid offset and size",
}; // static initialization }}}

// wsp_strerror {{{
const char *wsp_strerror(
    wsp_error_t *e
)
{
    return wsp_error_strings[e->type];
} // wsp_strerror }}}

// wsp_open {{{
wsp_return_t wsp_open(
    wsp_t *w,
    const char *path,
    wsp_mapping_t mapping,
    int flags,
    wsp_error_t *e
)
{
    if (w->io_instance != NULL) {
        e->type = WSP_ERROR_ALREADY_OPEN;
        return WSP_ERROR;
    }

    wsp_io *io = __wsp_get_io(mapping);

    if (io == NULL) {
        e->type = WSP_ERROR_IO;
        return WSP_ERROR;
    }

    w->io = io;

    if (w->io->open(w, path, flags, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    wsp_metadata_t meta;
    WSP_METADATA_INIT(&meta);

    if (__wsp_read_metadata(w, &meta, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    w->meta = meta;
    w->archives = NULL;
    w->archives_size = sizeof(wsp_archive_t) * meta.archives_count;
    w->archives_count = 0;

    if (__wsp_load_archives(w, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    return WSP_OK;
} // wsp_open }}}

// wsp_create {{{
wsp_return_t wsp_create(
    const char *path,
    wsp_archive_input_t *archives,
    size_t count,
    wsp_aggregation_t aggregation,
    float x_files_factor,
    wsp_mapping_t mapping,
    wsp_error_t *e
)
{
    wsp_io *io = __wsp_get_io(mapping);

    if (io == NULL) {
        e->type = WSP_ERROR_IO;
        return WSP_ERROR;
    }

    if (io->create == NULL) {
        e->type = WSP_ERROR_IO;
        return WSP_ERROR;
    }

    off_t size = sizeof(wsp_metadata_b) + sizeof(wsp_archive_b) * count;

    uint32_t max_retention = 0;

    wsp_metadata_t created_metadata = {
        .aggregation = aggregation,
        .max_retention = max_retention,
        .x_files_factor = x_files_factor,
        .archives_count = count
    };

    wsp_archive_t created_archives[count];

    uint32_t i;
    for (i = 0; i < count; i++) {
        wsp_archive_input_t *input = archives + i;
        wsp_archive_t *created = created_archives + i;

        size_t points_size = sizeof(wsp_point_b) * input->count;

        if (input->count <= 0) {
            e->type = WSP_ERROR_ARCHIVE;
            return WSP_ERROR;
        }

        if (input->spp <= 0) {
            e->type = WSP_ERROR_ARCHIVE;
            return WSP_ERROR;
        }

        created->offset = size;
        created->spp = input->spp;
        created->count = input->count;

        uint32_t retention = input->spp * input->count;

        if (max_retention < retention) {
            max_retention = retention;
        }

        size += points_size;
    }

    return io->create(path, size, created_archives, count, &created_metadata, e);
}
// wsp_create }}}

// wsp_close {{{
wsp_return_t wsp_close(wsp_t *w, wsp_error_t *e)
{
    if (w->archives != NULL) {
        uint32_t i;

        wsp_metadata_t m = w->meta;

        for (i = 0; i < m.archives_count; i++) {
            wsp_archive_t *archive = w->archives + i;

            if (__wsp_archive_free(archive, e) == WSP_ERROR) {
                return WSP_ERROR;
            }
        }

        free(w->archives);
        w->archives = NULL;
    }

    if (w->io->close(w, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    w->archives = NULL;
    w->archives_size = 0;
    w->meta.aggregation = 0l;
    w->meta.max_retention = 0l;
    w->meta.x_files_factor = 0.0f;
    w->meta.archives_count = 0l;

    return WSP_OK;
} // wsp_close }}}

// wsp_fetch_time_points {{{
wsp_return_t wsp_fetch_time_points(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t time_from,
    wsp_time_t time_until,
    wsp_point_t *result,
    uint32_t *size,
    wsp_error_t *e
)
{
    wsp_point_t base;

    if (__wsp_load_point(w, archive, 0, &base, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    if (!(time_from <= time_until)) {
        e->type = WSP_ERROR_TIME_INTERVAL;
        return WSP_ERROR;
    }

    uint32_t from = wsp_time_floor(time_from, archive->spp) / archive->spp;
    uint32_t until = wsp_time_floor(time_until, archive->spp) / archive->spp;
    int offset = from - (base.timestamp / archive->spp);

    uint32_t count = until - from + 1;

    if (DEBUG) {
        DEBUG_PRINTF(
            "wsp_fetch_time_points: offset=%d, count=%u", offset, count
        );
    }

    if (count > archive->count) {
        count = archive->count;
    }

    if (wsp_fetch_points(w, archive, offset, count, result, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    *size = count;

    return WSP_OK;
} // wsp_fetch_time_points }}}

// wsp_fetch_points {{{
wsp_return_t wsp_fetch_points(
    wsp_t *w,
    wsp_archive_t *archive,
    int offset,
    uint32_t count,
    wsp_point_t *result,
    wsp_error_t *e
)
{
    wsp_point_t base;

    if (__wsp_load_point(w, archive, 0, &base, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    if (count >= archive->count) {
        count = archive->count;
    }

    uint32_t from = __wsp_point_mod(offset, archive->count);
    uint32_t until = __wsp_point_mod(offset + count, archive->count);

    wsp_point_t points[count];

    if (__wsp_fetch_read_points(w, archive, from, until, count, points, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    if (__wsp_filter_points(&base, archive, offset, count, points, result, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    return WSP_OK;
} // wsp_fetch_points }}}

// wsp_load_points {{{
wsp_return_t wsp_load_points(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t offset,
    uint32_t size,
    wsp_point_t *result,
    wsp_error_t *e
)
{
    if (DEBUG) {
        DEBUG_PRINTF("offset=%u, size=%u", offset, size);
    }

    size_t read_offset = archive->offset + sizeof(wsp_point_b) * offset;
    size_t read_size = sizeof(wsp_point_b) * size;

    wsp_point_b *buf = NULL;

    if (w->io->read(w, read_offset, read_size, (void **)&buf, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    __wsp_parse_points(buf, size, result);

    if (w->io_manual_buf) {
        free(buf);
    }

    return WSP_OK;
} // wsp_load_points }}}

// wsp_point_index {{{
inline static uint32_t wsp_point_index(
    wsp_archive_t *archive,
    wsp_point_t *base,
    wsp_time_t floored
)
{
    wsp_time_t distance = floored - base->timestamp;
    return (distance / archive->spp) % archive->count;
} // wsp_point_index }}}

// wsp_update_point {{{
wsp_return_t wsp_update_point(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t time,
    wsp_value_t value,
    wsp_point_t *base,
    wsp_error_t *e
)
{
    wsp_point_t base_point;
    WSP_POINT_INIT(&base_point);

    if (__wsp_load_point(w, archive, 0, &base_point, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    uint32_t write_index = 0;

    wsp_time_t floored = wsp_time_floor(time, archive->spp);

    /* this not is the first point being written */
    if (base_point.timestamp != 0) {
        write_index = wsp_point_index(archive, &base_point, floored);
    }

    wsp_point_t p = { .timestamp = floored, .value = value };

    if (__wsp_save_point(w, archive, write_index, &p, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    *base = base_point;

    return WSP_OK;
} // wsp_update_point }}}

// wsp_update {{{
wsp_return_t wsp_update(
    wsp_t *w,
    wsp_point_input_t *point,
    wsp_error_t *e
)
{
    wsp_time_t now = wsp_time_now();

    wsp_time_t timestamp = point->timestamp;
    wsp_value_t value = point->value;

    if (timestamp == 0) {
        timestamp = now;
    }

    if (timestamp > now) {
        e->type = WSP_ERROR_FUTURE_TIMESTAMP;
        return WSP_ERROR;
    }

    wsp_time_t diff = now - timestamp;

    wsp_archive_t *low = NULL;
    uint32_t low_size = 0;

    if (__wsp_find_highest_precision(diff, w, &low, &low_size, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    wsp_point_t prev_base;

    if (wsp_update_point(w, low, timestamp, value, &prev_base, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    uint32_t i = 0;
    int skip = 0;

    wsp_archive_t *prev = low;

    // Propagate changes to lower precision archive.
    for (i = 1; i < low_size; i++) {
        wsp_archive_t *cur = low + i;

        wsp_time_t floor = wsp_time_floor(timestamp, cur->spp);
        uint32_t prev_index = wsp_point_index(prev, &prev_base, floor);
        uint32_t prev_count = cur->spp / prev->spp;

        wsp_point_t prev_points[prev_count];

        // Load array of points from the previous archive of points.
        if (wsp_load_points(w, prev, prev_index, prev_count, prev_points, e) == WSP_ERROR) {
            return WSP_ERROR;
        }

        wsp_value_t value = 0;

        if (w->meta.aggregate(w, prev_points, prev_count, &value, &skip, e) == WSP_ERROR) {
            return WSP_ERROR;
        }

        if (skip) {
            break;
        }

        if (wsp_update_point(w, cur, timestamp, value, &prev_base, e) == WSP_ERROR) {
            return WSP_ERROR;
        }

        prev = cur;
    }

    return WSP_OK;
} // wsp_update }}}

// wsp_parse_factor {{{
wsp_return_t wsp_parse_factor(
    const char *string,
    size_t length,
    int *result
)
{
    if (length <= 0) {
        return 0;
    }

    switch (string[length - 1]) {
    case 'y':
        *result = 3600 * 24 * 365;
        break;
    case 'w':
        *result = 3600 * 24 * 7;
        break;
    case 'd':
        *result = 3600 * 24;
        break;
    case 'h':
        *result = 3600;
        break;
    case 'm':
        *result = 60;
        break;
    default:
        return WSP_ERROR;
    }

    return WSP_OK;
} // wsp_parse_factor }}}

// wsp_parse_archive_input {{{
wsp_return_t wsp_parse_archive_input(
    const char *string,
    wsp_archive_input_t *archive
)
{
    size_t spp_l = 0;
    char spp_s[64];
    size_t points_l = 0;
    char count_s[64];

    while (*string != ':' && *string != '\0' && spp_l < sizeof(spp_s)) {
        spp_s[spp_l++] = *(string++);
    }

    if (*string != ':' || spp_l == 0) {
        return WSP_ERROR;
    }

    string++;

    while (*string != '\0' && points_l < sizeof(count_s)) {
        count_s[points_l++] = *(string++);
    }

    if (*string != '\0' || points_l == 0) {
        return WSP_ERROR;
    }

    spp_s[spp_l] = '\0';
    count_s[points_l] = '\0';

    int factor;

    if (wsp_parse_factor(spp_s, spp_l, &factor) == WSP_ERROR) {
        return WSP_ERROR;
    }

    if (factor == 0) {
        return WSP_ERROR;
    }

    if (factor != 1) {
        spp_s[spp_l - 1] = '\0';
    }

    size_t spp = strtol(spp_s, NULL, 10) * factor;
    size_t count = strtol(count_s, NULL, 10);

    if (spp == 0 || count == 0) {
        return WSP_ERROR;
    }

    archive->spp = spp;
    archive->count = count;

    return WSP_OK;
} // wsp_parse_archive_input }}}

wsp_return_t wsp_parse_point_input(
    const char *string,
    wsp_point_input_t *point
)
{
    size_t timestamp_l = 0;
    char timestamp_s[64];
    size_t points_l = 0;
    char value_s[64];

    while (*string != ':' && *string != '\0' && timestamp_l < sizeof(timestamp_s)) {
        timestamp_s[timestamp_l++] = *(string++);
    }

    if (*string != ':' || timestamp_l == 0) {
        return WSP_ERROR;
    }

    string++;

    while (*string != '\0' && points_l < sizeof(value_s)) {
        value_s[points_l++] = *(string++);
    }

    if (*string != '\0' || points_l == 0) {
        return WSP_ERROR;
    }

    timestamp_s[timestamp_l] = '\0';
    value_s[points_l] = '\0';

    size_t timestamp_i = strtol(timestamp_s, NULL, 10);
    double value = strtod(value_s, NULL);
    wsp_time_t timestamp = wsp_time_from_timestamp(timestamp_i);

    point->timestamp = timestamp;
    point->value = value;

    return WSP_OK;
}
