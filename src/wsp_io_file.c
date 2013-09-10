// vim: foldmethod=marker
#include "wsp_io_file.h"

#include <stdlib.h>
#include <errno.h>

/*
 * Open function for WSP_FILE mappings.
 *
 * See wsp_open_f for documentation on arguments.
 */
// __wsp_io_open__file {{{
static int __wsp_io_open__file(
    wsp_t *w,
    const char *path,
    int flags,
    wsp_error_t *e
)
{
    const char *mode;

    if (flags & WSP_READ && flags & WSP_WRITE) {
        mode = "rb+";
    }
    else if (flags & WSP_READ) {
        mode = "rb";
    }
    else if (flags & WSP_WRITE) {
        mode = "wb";
    }
    else {
        e->type = WSP_ERROR_IO_MODE;
        return WSP_ERROR;
    }

    FILE *io_fd = fopen(path, mode);

    if (!io_fd) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    w->io_fd = io_fd;
    w->io_mmap = NULL;
    w->io_size = 0;
    w->io_mapping = WSP_FILE;
    w->io_manual_buf = 1;
    w->io = &wsp_io_file;

    return WSP_OK;
} // __wsp_io_open__file }}}

/*
 * Close function for WSP_FILE mappings.
 *
 * See wsp_close_f for documentation on arguments.
 */
// __wsp_io_close__file {{{
static int __wsp_io_close__file(
    wsp_t *w,
    wsp_error_t *e
)
{
    if (w->io_fd != NULL) {
        fclose(w->io_fd);
        w->io_fd = NULL;
    }

    return WSP_OK;
} // __wsp_io_close__file }}}

/*
 * No memory allocation reader function for WSP_FILE mappings.
 *
 * See wsp_read_into_f for documentation on arguments.
 */
// __wsp_io_read_into__file {{{
static int __wsp_io_read_into__file(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    if (ftell(w->io_fd) != offset) {
        e->type = WSP_ERROR_OFFSET;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fread(buf, size, 1, w->io_fd) != 1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    return WSP_OK;
} // __wsp_io_read_into__file }}}

/*
 * Reader function for WSP_FILE mappings.
 *
 * See wsp_read_f for documentation on arguments.
 */
// __wsp_io_read__file {{{
static int __wsp_io_read__file(
    wsp_t *w,
    long offset,
    size_t size,
    void **buf,
    wsp_error_t *e
)
{
    void *tmp = malloc(size);

    if (tmp == NULL) {
        e->type = WSP_ERROR_MALLOC;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (ftell(w->io_fd) != offset) {
        free(tmp);
        e->type = WSP_ERROR_OFFSET;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fread(tmp, size, 1, w->io_fd) != 1) {
        free(tmp);
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    *buf = tmp;

    return WSP_OK;
} // __wsp_io_read__file }}}

/*
 * Writer function for WSP_FILE mappings.
 *
 * See wsp_write_f for documentation on arguments.
 */
// __wsp_io_write__file {{{
static int __wsp_io_write__file(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    if (fseek(w->io_fd, offset, SEEK_SET) == -1) {
        e->type = WSP_ERROR_OFFSET;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fwrite(buf, size, 1, w->io_fd) != 1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    return WSP_OK;
} // __wsp_io_write__file }}}

wsp_io wsp_io_file = {
    .open = __wsp_io_open__file,
    .close = __wsp_io_close__file,
    .read = __wsp_io_read__file,
    .read_into = __wsp_io_read_into__file,
    .write = __wsp_io_write__file,
    .create = NULL
};
