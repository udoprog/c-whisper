#include "wsp_io_file.h"

#include <stdlib.h>
#include <errno.h>

static int __wsp_io_open__file(
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

    w->io_fd = io_fd;
    w->io_mmap = NULL;
    w->io_size = 0;
    w->io_mapping = WSP_FILE;
    w->io_manual_buf = 1;
    w->io = &wsp_io_file;

    return WSP_OK;
}

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
}

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
} // __wsp_io_read__file

/*
 * Reader function for WSP_FILE mappings.
 *
 * See wsp_read_f for documentation on arguments.
 */
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
} // __wsp_io_read__file

/*
 * Writer function for WSP_FILE mappings.
 *
 * See wsp_write_f for documentation on arguments.
 */
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
} // __wsp_io_write__file

wsp_io wsp_io_file = {
    .open = __wsp_io_open__file,
    .close = __wsp_io_close__file,
    .read = __wsp_io_read__file,
    .read_into = __wsp_io_read_into__file,
    .write = __wsp_io_write__file
};
