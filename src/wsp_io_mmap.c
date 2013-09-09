// vim: foldmethod=marker
#include "wsp_io_mmap.h"

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

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

wsp_io wsp_io_mmap = {
    .open = __wsp_io_open__mmap,
    .close = __wsp_io_close__mmap,
    .read = __wsp_io_read__mmap,
    .read_into = __wsp_io_read_into__mmap,
    .write = __wsp_io_write__mmap,
};
