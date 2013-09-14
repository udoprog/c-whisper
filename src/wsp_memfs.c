// vim: foldmethod=marker
#include "wsp_memfs.h"

#include <stdlib.h>
#include <string.h>

wsp_memfs_t *wsp_memfs_find(
    wsp_memfs_context_t *ctx,
    const char *name
)
{
    wsp_memfs_t *current = ctx->first;

    while (current != NULL) {
        if (strncmp(current->name, name, sizeof(current->name)) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

void wsp_memfs_append(
    wsp_memfs_context_t *ctx,
    const char *name,
    void *memory,
    size_t size
)
{
    wsp_memfs_t *mf = wsp_memfs_find(ctx, name);

    if (mf != NULL) {
        free(mf->memory);
        strncpy(mf->name, name, sizeof(mf->name));
        mf->memory = memory;
        mf->size = size;
        return;
    }

    mf = wsp_memfs_new(ctx, name, memory, size);

    if (mf == NULL) {
        return;
    }

    if (ctx->last == NULL) {
        ctx->first = mf;
        ctx->last = mf;
        return;
    }

    ctx->last->next = mf;
    ctx->last = mf;
}

wsp_memfs_t *wsp_memfs_new(
    wsp_memfs_context_t *ctx,
    const char *name,
    void *memory,
    size_t size
)
{
    wsp_memfs_t *file = malloc(sizeof(wsp_memfs_t));

    if (file == NULL) {
        return NULL;
    }

    file->next = NULL;
    file->memory = memory;
    file->size = size;
    strncpy(file->name, name, sizeof(file->name));

    return file;
}

