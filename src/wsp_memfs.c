// vim: foldmethod=marker
#include "wsp_memfs.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    wsp_memfs_t *first;
    wsp_memfs_t *last;
} wsp_io_memory_context_t;

wsp_io_memory_context_t context = {
    .first = NULL,
    .last = NULL
};

wsp_memfs_t *wsp_memfs_find(
    const char *name
)
{
    wsp_memfs_t *current = context.first;

    while (current != NULL) {
        if (strncmp(current->name, name, sizeof(current->name)) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

void wsp_memfs_append(
    const char *name,
    void *memory,
    size_t size
)
{
    wsp_memfs_t *mf = wsp_memfs_find(name);

    if (mf != NULL) {
        free(mf->memory);
        strncpy(mf->name, name, sizeof(mf->name));
        mf->memory = memory;
        mf->size = size;
        return;
    }

    mf = wsp_memfs_new(name, memory, size);

    if (context.last == NULL) {
        context.first = mf;
        context.last = mf;
        return;
    }

    context.last->next = mf;
    context.last = mf;
}

wsp_memfs_t *wsp_memfs_new(
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

