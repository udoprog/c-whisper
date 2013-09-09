// vim: foldmethod=marker
#include "wsp_io_mmap.h"

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>

// __wsp_io_open__mmap {{{
static int __wsp_io_open__mmap(
    wsp_t *w,
    const char *path,
    wsp_error_t *e
)
{
    FILE *io_fd = fopen(path, "r+");

    if (!io_fd) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    struct stat st;

    int fn = fileno(io_fd);

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

    w->io_fd = io_fd;
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
    if (w->io_fd != NULL) {
        fclose(w->io_fd);
        w->io_fd = NULL;
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
