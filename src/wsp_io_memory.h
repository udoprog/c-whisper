#ifndef _WSP_IO_MEMORY_H_
#define _WSP_IO_MEMORY_H_

#include "wsp.h"
#include "wsp_memfs.h"

extern wsp_io wsp_io_memory;

typedef struct {
    wsp_memfs_t *file;
} wsp_io_memory_inst_t;

extern wsp_memfs_context_t memfs_ctx;

#endif /* _WSP_IO_MEMORY_H_ */
