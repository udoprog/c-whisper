// vim: foldmethod=marker
#include "wsp.h"

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "wsp_io_file.h"
#include "wsp_io_mmap.h"
#include "wsp_io_memory.h"

#include "wsp_debug.h"
#include "wsp_buffer.h"

// aggregate functions {{{
static wsp_return_t __wsp_aggregate_average(
    wsp_t *w,
    wsp_point_t *points,
    uint32_t count,
    double *value,
    int *skip,
    wsp_error_t *e
)
{
    if (count == 0) {
        *value = NAN;
        return WSP_OK;
    }

    uint32_t valid = 0;
    uint32_t i = 0;
    double total = 0;

    for (i = 0; i < count; i++) {
        wsp_point_t *p = points + i;

        double c = p->value;

        if (isnan(c)) {
            continue;
        }

        ++valid;

        total += c;
    }

    float known = (float)valid / (float)count;

    if (known < w->meta.x_files_factor) {
        *value = NAN;
        *skip = 1;
        return WSP_OK;
    }

    *value = total / valid;

    return WSP_OK;
}

static wsp_return_t __wsp_aggregate_sum(
    wsp_t *w,
    wsp_point_t *points,
    uint32_t count,
    double *value,
    int *skip,
    wsp_error_t *e
)
{
    if (count == 0) {
        *value = NAN;
        return WSP_OK;
    }

    uint32_t valid = 0;
    uint32_t i = 0;
    double total = 0;

    for (i = 0; i < count; i++) {
        wsp_point_t *p = points + i;

        double c = p->value;

        if (isnan(c)) {
            continue;
        }

        ++valid;

        total += c;
    }

    float known = (float)valid / (float)count;

    if (known < w->meta.x_files_factor) {
        *value = NAN;
        *skip = 1;
        return WSP_OK;
    }

    *value = total;

    return WSP_OK;
}

static wsp_return_t __wsp_aggregate_last(
    wsp_t *w,
    wsp_point_t *points,
    uint32_t count,
    double *value,
    int *skip,
    wsp_error_t *e
)
{
    if (count == 0) {
        *value = NAN;
        return WSP_OK;
    }

    *value = (points + count - 1)->value;
    return WSP_OK;
}

static wsp_return_t __wsp_aggregate_max(
    wsp_t *w,
    wsp_point_t *points,
    uint32_t count,
    double *value,
    int *skip,
    wsp_error_t *e
)
{
    if (count == 0) {
        *value = NAN;
        return WSP_OK;
    }

    uint32_t valid = 0;
    uint32_t i = 0;
    double max = NAN;

    for (i = 0; i < count; i++) {
        wsp_point_t *p = points + i;

        double c = p->value;

        if (isnan(c)) {
            continue;
        }

        ++valid;

        if (!isnan(max) && max >= c) {
            continue;
        }

        max = c;
    }

    float known = (float)valid / (float)count;

    if (known < w->meta.x_files_factor) {
        *value = NAN;
        *skip = 1;
        return WSP_OK;
    }

    *value = max;

    return WSP_OK;
}

static wsp_return_t __wsp_aggregate_min(
    wsp_t *w,
    wsp_point_t *points,
    uint32_t count,
    double *value,
    int *skip,
    wsp_error_t *e
)
{
    if (count == 0) {
        *value = NAN;
        return WSP_OK;
    }

    uint32_t valid = 0;
    uint32_t i = 0;
    double min = NAN;

    for (i = 0; i < count; i++) {
        wsp_point_t *p = points + i;

        double c = p->value;

        if (isnan(c)) {
            continue;
        }

        ++valid;

        if (!isnan(min) && min <= c) {
            continue;
        }

        min = c;
    }

    float known = (float)valid / (float)count;

    if (known < w->meta.x_files_factor) {
        *value = NAN;
        *skip = 1;
        return WSP_OK;
    }

    *value = min;

    return WSP_OK;
}
// }}}

// __wsp_parse_points {{{
void __wsp_parse_points(
    wsp_point_b *buf,
    uint32_t count,
    wsp_point_t *points
)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        __wsp_parse_point(buf + i, points + i);
    }
} // __wsp_parse_points }}}

// __wsp_dump_points {{{
void __wsp_dump_points(
    wsp_point_t *points,
    uint32_t count,
    wsp_point_b *buf
)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        __wsp_dump_point(points + i, buf + i);
    }
} // __wsp_dump_points }}}

// __wsp_read_metadata {{{
wsp_return_t __wsp_read_metadata(
    wsp_t *w,
    wsp_metadata_t *m,
    wsp_error_t *e
)
{
    wsp_metadata_b *buf;

    if (w->io->read(w, 0, sizeof(wsp_metadata_b), (void **)&buf, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    wsp_metadata_t tmp;

    __wsp_parse_metadata(buf, &tmp);

    if (w->io_manual_buf) {
        free(buf);
    }

    wsp_aggregate_f f = NULL;

    switch (tmp.aggregation) {
    case WSP_AVERAGE:
        f = __wsp_aggregate_average;
        break;
    case WSP_SUM:
        f = __wsp_aggregate_sum;
        break;
    case WSP_LAST:
        f = __wsp_aggregate_last;
        break;
    case WSP_MAX:
        f = __wsp_aggregate_max;
        break;
    case WSP_MIN:
        f = __wsp_aggregate_min;
        break;
    default:
        e->type = WSP_ERROR_UNKNOWN_AGGREGATION;
        return WSP_ERROR;
    }

    m->aggregation = tmp.aggregation;
    m->max_retention = tmp.max_retention;
    m->x_files_factor = tmp.x_files_factor;
    m->archives_count = tmp.archives_count;
    m->aggregate = f;

    return WSP_OK;
} // __wsp_read_metadata }}}

// __wsp_read_archive {{{
wsp_return_t __wsp_read_archive(
    wsp_t *w,
    int index,
    wsp_archive_t *archive,
    wsp_error_t *e
)
{
    wsp_archive_b *buf;

    size_t offset = WSP_ARCHIVE_OFFSET(index);

    if (w->io->read(w, offset, sizeof(wsp_archive_b), (void **)&buf, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    __wsp_parse_archive(buf, archive);

    if (w->io_manual_buf) {
        free(buf);
    }

    archive->points_size = sizeof(wsp_point_t) * archive->count;
    archive->retention = archive->spp * archive->count;

    return WSP_OK;
} // __wsp_read_archive }}}

// __wsp_valid_archive {{{
wsp_return_t __wsp_valid_archive(
    wsp_archive_t *prev,
    wsp_archive_t *cur,
    wsp_error_t *e
)
{
    // validate archive.
    if (cur->spp % prev->spp != 0) {
        e->type = WSP_ERROR_ARCHIVE_MISALIGNED;
        return WSP_ERROR;
    }

    if (cur->count % prev->count != 0) {
        e->type = WSP_ERROR_ARCHIVE_MISALIGNED;
        return WSP_ERROR;
    }

    return WSP_OK;
} // __wsp_valid_archive }}}

// __wsp_load_archives {{{
wsp_return_t __wsp_load_archives(
    wsp_t *w,
    wsp_error_t *e
)
{
    wsp_archive_t *archives = malloc(w->archives_size);

    if (!archives) {
        e->type = WSP_ERROR_MALLOC;
        return WSP_ERROR;
    }

    uint32_t i;

#ifdef VALIDATE_ARCHIVE
    wsp_archive_t *prev = NULL;
#endif /* VALIDATE_ARCHIVE */

    for (i = 0; i < w->meta.archives_count; i++) {
        wsp_archive_t *cur = archives + i;

        if (__wsp_read_archive(w, i, cur, e) == WSP_ERROR) {
            free(archives);
            return WSP_ERROR;
        }

#ifdef VALIDATE_ARCHIVE
        if (prev != NULL) {
            if (__wsp_valid_archive(prev, cur, e) == WSP_ERROR) {
                free(archives);
                return WSP_ERROR;
            }
        }

        prev = cur;
#endif /* VALIDATE_ARCHIVE */
    }

    // free any old archive.
    if (w->archives != NULL) {
        free(w->archives);
        w->archives = NULL;
        w->archives_count = 0;
    }

    w->archives = archives;
    w->archives_count = w->meta.archives_count;

    return WSP_OK;
} // __wsp_load_archives }}}

// __wsp_archive_free {{{
wsp_return_t __wsp_archive_free(
    wsp_archive_t *archive,
    wsp_error_t *e
)
{
    archive->offset = 0;
    archive->spp = 0;
    archive->count = 0;
    archive->points_size = 0;

    return WSP_OK;
} // __wsp_archive_free }}}

// __wsp_find_highest_precision {{{
wsp_return_t __wsp_find_highest_precision(
    wsp_time_t diff,
    wsp_t *w,
    wsp_archive_t **low,
    uint32_t *low_size,
    wsp_error_t *e
)
{
    wsp_time_t max_retention = (wsp_time_t)w->meta.max_retention;

    if (diff >= max_retention) {
        e->type = WSP_ERROR_RETENTION;
        return WSP_ERROR;
    }

    uint32_t index = 0;

    for (index = 0; index < w->archives_count; index++) {
        wsp_archive_t *archive =  w->archives + index;

        if (archive->retention < diff) {
            continue;
        }

        break;
    }

    uint32_t size = w->archives_count - index;

    if (size == 0) {
        e->type = WSP_ERROR_ARCHIVE;
        return WSP_ERROR;
    }

    *low = w->archives + index;
    *low_size = w->archives_count - index;

    return WSP_OK;
} // __wsp_find_highest_precision }}}

// __wsp_write_segment {{{
inline static wsp_return_t __wsp_write_segment(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_point_t *points,
    size_t length,
    size_t position,
    wsp_error_t *e
)
{
    wsp_point_b buf[length];

    size_t write_offset = WSP_POINT_OFFSET(archive, position);
    size_t write_size = sizeof(wsp_point_b) * length;

    __wsp_dump_points(points, length, buf);

    if (w->io->write(w, write_offset, write_size, (void *)buf, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    return WSP_OK;
}
// __wsp_write_segment }}}

// __wsp_save_points {{{
wsp_return_t __wsp_save_points(
    wsp_t *w,
    wsp_archive_t *archive,
    long offset,
    wsp_point_t *points,
    size_t length,
    wsp_error_t *e
)
{
    if (length >= archive->count) {
        e->type = WSP_ERROR_POINT_OOB;
        return WSP_ERROR;
    }

    // One write.
    if (offset + length <= archive->count) {
        if (__wsp_write_segment(w, archive, points, length, offset, e) == WSP_ERROR) {
            return WSP_ERROR;
        }
    }
    // Two writes.
    else {
        size_t write_length_a = archive->count - offset;
        size_t write_length_b = archive->count - write_length_a;

        if (__wsp_write_segment(w, archive, points, write_length_a, offset, e) == WSP_ERROR) {
            return WSP_ERROR;
        }

        if (__wsp_write_segment(w, archive, points + write_length_a, write_length_b, 0, e) == WSP_ERROR) {
            return WSP_ERROR;
        }
    }

    return WSP_OK;
} // __wsp_save_points }}}

// __wsp_load_point {{{
wsp_return_t __wsp_load_point(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t index,
    wsp_point_t *point,
    wsp_error_t *e
)
{
    wsp_point_b buf;

    size_t read_offset = WSP_POINT_OFFSET(archive, index);
    size_t read_size = sizeof(wsp_point_b);

    if (w->io->read_into(w, read_offset, read_size, &buf, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    __wsp_parse_point(&buf, point);

    return WSP_OK;
} // __wsp_load_point }}}

// __wsp_point_mod {{{
uint32_t __wsp_point_mod(int value, uint32_t div)
{
    int result = value % ((int)div);

    if (result < 0) {
        result += div;
    }

    return (uint32_t)result;
} // __wsp_point_mod }}}

// __wsp_filter_points {{{
wsp_return_t __wsp_filter_points(
    wsp_point_t *base,
    wsp_archive_t *archive,
    int offset,
    uint32_t count,
    wsp_point_t *points,
    wsp_point_t *result,
    wsp_error_t *e
)
{
    uint32_t spp = archive->spp;
    uint32_t counter = base->timestamp + (spp * offset);

    uint32_t i;
    for (i = 0; i < count; i++) {
        wsp_point_t p = points[i];

        if (DEBUG) {
            DEBUG_PRINTF(
                "compare: %u, %u: %s",
                counter, p.timestamp, to_bool(counter == p.timestamp)
            );
        }

        if (p.timestamp == counter) {
            result[i] = p;
        }
        else {
            wsp_point_t zero = { .timestamp = counter, .value = NAN };
            result[i] = zero;
        }

        counter += spp;
    }

    return WSP_OK;
} // __wsp_filter_points }}}

// __wsp_fetch_read_points {{{
wsp_return_t __wsp_fetch_read_points(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t from,
    uint32_t until,
    uint32_t count,
    wsp_point_t *points,
    wsp_error_t *e
)
{
    if (DEBUG) {
        DEBUG_PRINTF("from=%u, until=%u, count=%u", from, until, count);
    }

    if (from != 0 && until <= from) {
        // wrap around
        uint32_t a_from = from;
        uint32_t a_size = archive->count - from;
        wsp_point_t *a_points = points;

        uint32_t b_from = 0;
        uint32_t b_size = until;
        wsp_point_t *b_points = points + a_size;

        if (wsp_load_points(w, archive, a_from, a_size, a_points, e) == WSP_ERROR) {
            return WSP_ERROR;
        }

        if (wsp_load_points(w, archive, b_from, b_size, b_points, e) == WSP_ERROR) {
            return WSP_ERROR;
        }
    }
    else {
        // single linear load.
        if (wsp_load_points(w, archive, from, count, points, e) == WSP_ERROR) {
            return WSP_ERROR;
        }
    }

    return WSP_OK;
} // __wsp_fetch_read_points }}}

// __wsp_get_io {{{
wsp_io *__wsp_get_io(wsp_mapping_t mapping)
{
    if (mapping == WSP_MMAP) {
        return &wsp_io_mmap;
    }

    if (mapping == WSP_FILE) {
        return &wsp_io_file;
    }

    if (mapping == WSP_MEMORY) {
        return &wsp_io_memory;
    }

    return NULL;
} // __wsp_get_io }}}

// __wsp_build_point {{{
void __wsp_build_point(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t time,
    wsp_value_t value,
    wsp_point_t *result
)
{
    result->timestamp = wsp_time_floor(time, archive->spp);
    result->value = value;
} // __wsp_build_point }}}
