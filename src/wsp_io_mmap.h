#ifndef _WSP_IO_MMAP_H_
#define _WSP_IO_MMAP_H_

#include "wsp.h"

extern wsp_io wsp_io_mmap;

typedef struct {
    void *map;
    size_t size;
    int fn;
} wsp_io_mmap_inst_t;

#endif /* _WSP_IO_MMAP_H_ */
