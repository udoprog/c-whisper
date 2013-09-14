// vim: foldmethod=marker
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "wsp_io_memory.h"
#include "wsp_private.h"
#include "wsp_debug.h"
#include "wsp_memfs.h"

wsp_memfs_context_t memfs_ctx = WSP_MEMFS_CONTEXT_INIT;

// __wsp_io_open__memory {{{
static int __wsp_io_open__memory(
    wsp_t *w,
    const char *path,
    int flags,
    wsp_error_t *e
)
{
    wsp_memfs_t *mf = wsp_memfs_find(&memfs_ctx, path);

    if (mf == NULL) {
        e->type = WSP_ERROR_IO;
        return WSP_ERROR;
    }

    wsp_io_memory_inst_t *self = malloc(sizeof(wsp_io_memory_inst_t));

    if (self == NULL) {
        e->type = WSP_ERROR_MALLOC;
        return WSP_ERROR;
    }

    self->file = mf;

    w->io_instance = self;

    return WSP_OK;
} // __wsp_io_open__memory }}}

// __wsp_io_close__memory {{{
static int __wsp_io_close__memory(
    wsp_t *w,
    wsp_error_t *e
)
{
    wsp_io_memory_inst_t *self;
    WSP_IO_CHECK(w, WSP_MEMORY, wsp_io_memory_inst_t, self, e);

    free(self);

    w->io_instance = NULL;

    return WSP_OK;
} // __wsp_io_close__memory }}}

/*
 * Reader function for WSP_MEMORY mappings.
 *
 * See wsp_read_f for documentation on arguments.
 */
// __wsp_io_read__memory {{{
static int __wsp_io_read__memory(
    wsp_t *w,
    long offset,
    size_t size,
    void **buf,
    wsp_error_t *e
)
{
    wsp_io_memory_inst_t *self;
    WSP_IO_CHECK(w, WSP_MEMORY, wsp_io_memory_inst_t, self, e);

    wsp_memfs_t *mf = self->file;

    if (offset + size > mf->size) {
        e->type = WSP_ERROR_IO_OFFSET;
        return WSP_ERROR;
    }

    /* the beauty of memoryed files */
    *buf = (char *)mf->memory + offset;
    return WSP_OK;
} // __wsp_io_read__memory }}}

/*
 * Reader function for WSP_MEMORY mappings.
 *
 * See wsp_read_into_f for documentation on arguments.
 */
// __wsp_io_read_into__memory {{{
static int __wsp_io_read_into__memory(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    wsp_io_memory_inst_t *self;
    WSP_IO_CHECK(w, WSP_MEMORY, wsp_io_memory_inst_t, self, e);

    wsp_memfs_t *mf = self->file;

    if (offset + size > mf->size) {
        e->type = WSP_ERROR_IO_OFFSET;
        return WSP_ERROR;
    }

    memcpy(buf, (char*)mf->memory + offset, size);
    return WSP_OK;
} // __wsp_read_into__memory }}}

/*
 * Writer function for WSP_MEMORY mappings.
 *
 * See wsp_write_f for documentation on arguments.
 */
// __wsp_io_write__memory {{{
static int __wsp_io_write__memory(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
)
{
    wsp_io_memory_inst_t *self;
    WSP_IO_CHECK(w, WSP_MEMORY, wsp_io_memory_inst_t, self, e);

    wsp_memfs_t *mf = self->file;

    if (offset + size > mf->size) {
        e->type = WSP_ERROR_IO_OFFSET;
        return WSP_ERROR;
    }

    memcpy((char *)mf->memory + offset, buf, size);
    return WSP_OK;
} // __wsp_io_write__memory }}}

/*
 * Create function for WSP_MEMORY mappings.
 */
// __wsp_io_create_generic {{{
wsp_return_t __wsp_io_create__memory(
    const char *path,
    size_t size,
    wsp_archive_t *created_archives,
    size_t count,
    wsp_metadata_t *metadata,
    wsp_error_t *e
)
{
    off_t archives_offset = sizeof(wsp_metadata_b);

    char *buf = malloc(size);

    if (buf == NULL) {
        e->type = WSP_ERROR_MALLOC;
        return WSP_ERROR;
    }

    __wsp_dump_metadata(metadata, (void *)buf);
    __wsp_dump_archives(created_archives, count, (void *)(buf + archives_offset));

    wsp_memfs_append(&memfs_ctx, path, buf, size);

    return WSP_OK;
} // __wsp_io_create__memory }}}

wsp_io wsp_io_memory = {
    .open = __wsp_io_open__memory,
    .close = __wsp_io_close__memory,
    .read = __wsp_io_read__memory,
    .read_into = __wsp_io_read_into__memory,
    .write = __wsp_io_write__memory,
    .create = __wsp_io_create__memory,
};
