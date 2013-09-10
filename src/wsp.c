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
    /* WSP_ERROR_NOT_INITIALIZED */
    "Context not initialized",
    /* WSP_ERROR_ALREADY_INITIALIZED */
    "Context already initialized",
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
    "Invalid open mode"
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
    if (w->io_fd != NULL || w->io_mmap != NULL) {
        e->type = WSP_ERROR_ALREADY_OPEN;
        return WSP_ERROR;
    }

    wsp_io *io = __wsp_get_io(mapping);

    if (io == NULL) {
        e->type = WSP_ERROR_IO;
        return WSP_ERROR;
    }

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
    wsp_archive_t *archives,
    size_t archives_length,
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

    return io->create(path, archives, archives_length, aggregation, x_files_factor, e);
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
    double value,
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
wsp_return_t wsp_update(wsp_t *w, wsp_point_t *p, wsp_error_t *e)
{
    wsp_time_t now = wsp_time_now();

    wsp_time_t timestamp = p->timestamp;
    double value = p->value;

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

        double value = 0;

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
