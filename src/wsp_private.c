// vim: foldmethod=marker
#include "wsp.h"

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

// inline parse & dump functions {{{
#if BYTE_ORDER == LITTLE_ENDIAN
#define READ4(t, l) do {\
    (t)[0] = (l)[3];\
    (t)[1] = (l)[2];\
    (t)[2] = (l)[1];\
    (t)[3] = (l)[0];\
} while (0);

#define READ8(t, l) do {\
    (t)[0] = (l)[7];\
    (t)[1] = (l)[6];\
    (t)[2] = (l)[5];\
    (t)[3] = (l)[4];\
    (t)[4] = (l)[3];\
    (t)[5] = (l)[2];\
    (t)[6] = (l)[1];\
    (t)[7] = (l)[0];\
} while (0);
#else
#define READ4(t, l) do {\
    (t)[0] = (l)[0];\
    (t)[1] = (l)[1];\
    (t)[2] = (l)[2];\
    (t)[3] = (l)[3];\
} while (0);

#define READ8(t, l) do {\
    (t)[0] = (l)[0];\
    (t)[1] = (l)[1];\
    (t)[2] = (l)[2];\
    (t)[3] = (l)[3];\
    (t)[4] = (l)[4];\
    (t)[5] = (l)[5];\
    (t)[6] = (l)[6];\
    (t)[7] = (l)[7];\
} while (0);
#endif

inline void __wsp_parse_point(
    wsp_point_b *buf,
    wsp_point_t *p
)
{
    READ4((char *)&p->timestamp, buf->timestamp);
    READ8((char *)&p->value, buf->value);
} // __wsp_parse_point

inline void __wsp_dump_point(
    wsp_point_t *p,
    wsp_point_b *buf
)
{
    READ4(buf->timestamp, (char *)&p->timestamp);
    READ8(buf->value, (char *)&p->value);
} // __wsp_dump_point

inline void __wsp_parse_points(
    wsp_point_b *buf,
    uint32_t count,
    wsp_point_t *points
)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        __wsp_parse_point(buf + i, points + i);
    }
} // __wsp_parse_points

inline void __wsp_dump_points(
    wsp_point_t *points,
    uint32_t count,
    wsp_point_b *buf
)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        __wsp_dump_point(points + i, buf + i);
    }
} // __wsp_dump_points

inline void __wsp_parse_metadata(
    wsp_metadata_b *buf,
    wsp_metadata_t *m
)
{
    READ4((char *)&m->aggregation, buf->aggregation);
    READ4((char *)&m->max_retention, buf->max_retention);
    READ4((char *)&m->x_files_factor, buf->x_files_factor);
    READ4((char *)&m->archives_count, buf->archives_count);
} // __wsp_parse_metadata

inline void __wsp_dump_metadata(
    wsp_metadata_t *m,
    wsp_metadata_b *buf
)
{
    READ4(buf->aggregation, (char *)&m->aggregation);
    READ4(buf->max_retention, (char *)&m->max_retention);
    READ4(buf->x_files_factor, (char *)&m->x_files_factor);
    READ4(buf->archives_count, (char *)&m->archives_count);
} // __wsp_dump_metadata

inline void __wsp_parse_archive(
    wsp_archive_b *buf,
    wsp_archive_t *archive
)
{
    READ4((char *)&archive->offset, buf->offset);
    READ4((char *)&archive->spp, buf->spp);
    READ4((char *)&archive->count, buf->count);
} // __wsp_parse_archive

inline void __wsp_dump_archive(
    wsp_archive_t *archive,
    wsp_archive_b *buf
)
{
    READ4(buf->offset, (char *)&archive->offset);
    READ4(buf->spp, (char *)&archive->spp);
    READ4(buf->count, (char *)&archive->count);
} // __wsp_dump_archive
// inline parse & dump functions }}}

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

        if (c == NAN) {
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

        if (c == NAN) {
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

        if (c == NAN) {
            continue;
        }

        ++valid;

        if (max != NAN && max >= c) {
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

        if (c == NAN) {
            continue;
        }

        ++valid;

        if (min != NAN && min <= c) {
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

/*
 * Setup memory mapping for the specified file.
 *
 * io_fd: The file descriptor to memory map.
 * io_mmap: Pointer to a pointer that will be written to the new memory region.
 * io_size: Pointer to write the file size to.
 * e: Error object.
 */
// __wsp_setup_mmap {{{
wsp_return_t __wsp_setup_mmap(
    FILE *io_fd,
    void **io_mmap,
    off_t *io_size,
    wsp_error_t *e
)
{
    int fn = fileno(io_fd);

    struct stat st;

    if (fstat(fn, &st) == -1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    void *tmp = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fn, 0);

    if (tmp == MAP_FAILED) {
        fclose(io_fd);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    *io_mmap = tmp;
    *io_size = st.st_size;
    return WSP_OK;
} // }}} __wsp_setup_mmap

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

// __wsp_save_point {{{
wsp_return_t __wsp_save_point(
    wsp_t *w,
    wsp_archive_t *archive,
    long index,
    wsp_point_t *point,
    wsp_error_t *e
)
{
    wsp_point_b buf;

    if (index >= archive->count) {
        e->type = WSP_ERROR_POINT_OOB;
        return WSP_ERROR;
    }

    size_t write_offset = WSP_POINT_OFFSET(archive, index);
    size_t write_size = sizeof(wsp_point_b);

    __wsp_dump_point(point, &buf);

    if (w->io->write(w, write_offset, write_size, (void *)&buf, e) == WSP_ERROR) {
        return WSP_ERROR;
    }

    return WSP_OK;
} // __wsp_save_point }}}

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

// __wsp_load_points {{{
int __wsp_load_points(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t offset,
    uint32_t size,
    wsp_point_t *result,
    wsp_error_t *e
)
{
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
} // __wsp_load_points }}}

// __wsp_point_mod {{{
uint32_t __wsp_point_mod(int value, uint32_t div)
{
    int result = value % ((int)div);

    if (result < 0) {
        result += div;
    }

    return (uint32_t)result;
} // __wsp_point_mod }}}
