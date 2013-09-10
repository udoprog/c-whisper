// vim: foldmethod=marker
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "wsp_io_file.h"
#include "wsp_private.h"

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

/*
 * Create function for WSP_FILE mappings.
 */
// __wsp_io_create_generic {{{
wsp_return_t __wsp_io_create__file(
    const char *path,
    size_t size,
    wsp_archive_t *created_archives,
    size_t count,
    wsp_metadata_t *metadata,
    wsp_error_t *e
)
{
    FILE *fp = fopen(path, "ab");

    if (fp == NULL) {
        e->type = WSP_ERROR_FOPEN;
        e->syserr = errno;
        return WSP_ERROR;
    }

    int fn = fileno(fp);

    if (fn == -1) {
        fclose(fp);
        e->type = WSP_ERROR_FILENO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (ftruncate(fn, size) == -1) {
        fclose(fp);
        e->type = WSP_ERROR_FTRUNCATE;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fflush(fp) == -1) {
        fclose(fp);
        e->type = WSP_ERROR_FSYNC;
        e->syserr = errno;
        return WSP_ERROR;
    }

    wsp_metadata_b metadata_buf;
    wsp_archive_b archives_buf[count];

    __wsp_dump_metadata(metadata, (void *)&metadata_buf);
    __wsp_dump_archives(created_archives, count, (void *)archives_buf);

    if (fseek(fp, 0, SEEK_SET) == -1) {
        e->type = WSP_ERROR_OFFSET;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fwrite(&metadata_buf, sizeof(wsp_metadata_b), 1, fp) != 1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    if (fwrite(archives_buf, sizeof(wsp_archive_b) * count, 1, fp) != 1) {
        e->type = WSP_ERROR_IO;
        e->syserr = errno;
        return WSP_ERROR;
    }

    fclose(fp);

    return WSP_OK;
} // __wsp_io_create__mmap }}}

wsp_io wsp_io_file = {
    .open = __wsp_io_open__file,
    .close = __wsp_io_close__file,
    .read = __wsp_io_read__file,
    .read_into = __wsp_io_read_into__file,
    .write = __wsp_io_write__file,
    .create = __wsp_io_create__file
};
