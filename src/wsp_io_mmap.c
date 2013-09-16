// vim: foldmethod=marker
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "wsp_io_mmap.h"
#include "wsp_private.h"
#include "wsp_debug.h"

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

    void *map = mmap(NULL, st.st_size, mmap_prot, MAP_SHARED, fn, 0);

    if (map == MAP_FAILED) {
        close(fn);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    wsp_io_mmap_inst_t *self = malloc(sizeof(wsp_io_mmap_inst_t));

    if (self == NULL) {
        close(fn);
        munmap(map, st.st_size);
        e->type = WSP_ERROR_MALLOC;
        e->syserr = errno;
        return WSP_ERROR;
    }

    self->map = map;
    self->size = st.st_size;
    self->fn = fn;

    w->io_instance = self;
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
    wsp_io_mmap_inst_t *self;
    WSP_IO_CHECK(w, WSP_MMAP, wsp_io_mmap_inst_t, self, e);

    close(self->fn);
    munmap(self->map, self->size);

    free(self);

    w->io_instance = NULL;
    return WSP_OK;
} // __wsp_io_close__mmap }}}

/*
 * Reader function for WSP_MMAP mappings.
 *
 * See wsp_read_f for documentation on arguments.
 */
// __wsp_io_read__mmap {{{
static int __wsp_io_read__mmap(
    wsp_t *w,
    long offset,
    size_t size,
    void **buf,
    wsp_error_t *e
)
{
    wsp_io_mmap_inst_t *self;
    WSP_IO_CHECK(w, WSP_MMAP, wsp_io_mmap_inst_t, self, e);

    /* the beauty of mmaped files */
    *buf = (char *)self->map + offset;
    return WSP_OK;
} // __wsp_io_read__mmap }}}

/*
 * Reader function for WSP_MMAP mappings.
 *
 * See wsp_read_into_f for documentation on arguments.
 */
// __wsp_io_read_into__mmap {{{
static int __wsp_io_read_into__mmap(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    wsp_io_mmap_inst_t *self;
    WSP_IO_CHECK(w, WSP_MMAP, wsp_io_mmap_inst_t, self, e);

    memcpy(buf, (char*)self->map + offset, size);
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
    wsp_io_mmap_inst_t *self;
    WSP_IO_CHECK(w, WSP_MMAP, wsp_io_mmap_inst_t, self, e);

    memcpy((char *)self->map + offset, buf, size);
    return WSP_OK;
} // __wsp_io_write__mmap }}}

/*
 * Create function for WSP_MMAP mappings.
 */
// __wsp_io_create_generic {{{
wsp_return_t __wsp_io_create__mmap(
    const char *path,
    size_t size,
    wsp_archive_t *created_archives,
    size_t count,
    wsp_metadata_t *metadata,
    wsp_error_t *e
)
{
    off_t archives_offset = sizeof(wsp_metadata_b);

    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

    int fn = open(path, O_CREAT | O_RDWR, mode);

    if (fn == -1) {
        e->type = WSP_ERROR_OPEN;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (DEBUG) {
        DEBUG_PRINTF("size = %zu", size);
    }

    if (ftruncate(fn, size) == -1) {
        close(fn);
        e->type = WSP_ERROR_FTRUNCATE;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fsync(fn) == -1) {
        e->type = WSP_ERROR_FSYNC;
        e->syserr = errno;
        return WSP_ERROR;
    }

    void *map = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fn, 0);

    if (map == MAP_FAILED) {
        close(fn);
        e->type = WSP_ERROR_MMAP;
        e->syserr = errno;
        return WSP_ERROR;
    }

    char *buf = (char *)map;

    __wsp_dump_metadata(metadata, (void *)buf);
    __wsp_dump_archives(created_archives, count, (void *)(buf + archives_offset));

    munmap(map, size);
    close(fn);

    return WSP_OK;
} // __wsp_io_create__mmap }}}

wsp_io wsp_io_mmap = {
    .open = __wsp_io_open__mmap,
    .close = __wsp_io_close__mmap,
    .read = __wsp_io_read__mmap,
    .read_into = __wsp_io_read_into__mmap,
    .write = __wsp_io_write__mmap,
    .create = __wsp_io_create__mmap,
};
