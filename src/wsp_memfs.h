// vim: foldmethod=marker
#ifndef _WSP_MEMFS_H_
#define _WSP_MEMFS_H_

#include <stdlib.h>

#define WSP_MEMFS_NAME_MAX 256

typedef struct wsp_memfs_s {
    char name[WSP_MEMFS_NAME_MAX];
    void *memory;
    size_t size;
    struct wsp_memfs_s *next;
} wsp_memfs_t;

wsp_memfs_t *wsp_memfs_find(
    const char *name
);

void wsp_memfs_append(
    const char *name,
    void *memory,
    size_t size
);

wsp_memfs_t *wsp_memfs_new(
    const char *name,
    void *memory,
    size_t size
);

#endif /* _WSP_MEMFS_H_ */
