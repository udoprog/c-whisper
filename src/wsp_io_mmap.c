// vim: foldmethod=marker
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "wsp_io_mmap.h"
#include "wsp_private.h"

// __wsp_io_open__mmap {{{
static int __wsp_io_open__mmap(
    wsp_t *w,
    const char *path,
    int flags,
    wsp_error_t *e
)
{
    int mmap_prot;
    int open_flags;

    if (flags & WSP_READ && flags & WSP_WRITE) {
        mmap_prot = PROT_READ | PROT_WRITE;
        open_flags = O_RDWR;
    }
    else if (flags & WSP_READ) {
        mmap_prot = PROT_READ;
        open_flags = O_RDONLY;
    }
    else if (flags & WSP_WRITE) {
        mmap_prot = PROT_WRITE;
        open_flags = O_WRONLY;
    }
    else {
        e->type = WSP_ERROR_IO_MODE;
        return WSP_ERROR;
    }

    int fn = open(path, open_flags);

    if (fn == -1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    struct stat st;

    if (fstat(fn, &st) == -1) {
        close(fn);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    void *tmp = mmap(NULL, st.st_size, mmap_prot, MAP_SHARED, fn, 0);

    if (tmp == MAP_FAILED) {
        close(fn);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    w->io_fd = NULL;
    w->io_fn = fn;
    w->io_mmap = tmp;
    w->io_size = st.st_size;
    w->io_mapping = WSP_MMAP;
    w->io_manual_buf = 0;

    return WSP_OK;
} // __wsp_io_open__mmap }}}

// __wsp_io_close__mmap {{{
static int __wsp_io_close__mmap(
    wsp_t *w,
    wsp_error_t *e
)
{
    if (w->io_fn != -1) {
        close(w->io_fn);
        w->io_fn = -1;
    }

    if (w->io_mmap != NULL) {
        munmap(w->io_mmap, w->io_size);
        w->io_mmap = NULL;
    }

    return WSP_OK;
} // __wsp_io_close__mmap }}}

/*
 * Reader function for WSP_MMAP mappings.
 *
 * See wsp_read_f for documentation on arguments.
 */
// __wsp_io_read__mmap {{{
static int __wsp_io_read__mmap(
    wsp_t *file,
    long offset,
    size_t size,
    void **buf,
    wsp_error_t *e
)
{
    /* the beauty of mmaped files */
    *buf = (char *)file->io_mmap + offset;
    return WSP_OK;
} // __wsp_io_read__mmap }}}

/*
 * Reader function for WSP_MMAP mappings.
 *
 * See wsp_read_into_f for documentation on arguments.
 */
// __wsp_io_read_into__mmap {{{
static int __wsp_io_read_into__mmap(
    wsp_t *file,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    memcpy(buf, (char*)file->io_mmap + offset, size);
    return WSP_OK;
} // __wsp_read_into__mmap }}}

/*
 * Writer function for WSP_MMAP mappings.
 *
 * See wsp_write_f for documentation on arguments.
 */
// __wsp_io_write__mmap {{{
static int __wsp_io_write__mmap(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    memcpy((char *)w->io_mmap + offset, buf, size);
    return WSP_OK;
} // __wsp_io_write__mmap }}}

// __wsp_io_create_generic
wsp_return_t __wsp_io_create__generic(
    const char *path,
    wsp_archive_t *archives,
    size_t count,
    wsp_aggregation_t aggregation,
    float x_files_factor,
    wsp_error_t *e
)
{
    int fn = open(path, O_CREAT);

    if (fn == -1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    off_t archives_offset = sizeof(wsp_metadata_b);

    off_t size = archives_offset + \
                 sizeof(wsp_archive_b) * count;

    uint32_t max_retention = 0;

    uint32_t i;
    for (i = 0; i < count; i++) {
        wsp_archive_t *archive = archives + i;
        size_t points_size = sizeof(wsp_point_b) * archive->count;

        archive->offset = size;

        uint32_t retention = archive->spp * archive->count;

        if (max_retention < retention) {
            max_retention = retention;
        }

        size += points_size;
    }

    if (ftruncate(fn, size) == -1) {
        close(fn);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    void *map = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fn, 0);

    if (map == MAP_FAILED) {
        close(fn);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    wsp_metadata_t metadata = {
        .aggregation = aggregation,
        .max_retention = max_retention,
        .x_files_factor = x_files_factor,
        .archives_count = count
    };

    char *buf = (char *)map;

    __wsp_dump_metadata(&metadata, (void *)buf);
    __wsp_dump_archives(archives, count, (void *)(buf + archives_offset));

    munmap(map, size);
    close(fn);

    return WSP_OK;
}

wsp_io wsp_io_mmap = {
    .open = __wsp_io_open__mmap,
    .close = __wsp_io_close__mmap,
    .read = __wsp_io_read__mmap,
    .read_into = __wsp_io_read_into__mmap,
    .write = __wsp_io_write__mmap,
    .create = __wsp_io_create__generic,
};
