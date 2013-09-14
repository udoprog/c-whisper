#ifndef _WSP_IO_FILE_H_
#define _WSP_IO_FILE_H_

#include "wsp.h"

/**
 * Regular file based I/O, this has the benefit of being insanely portable but
 * typically slower.
 */
extern wsp_io wsp_io_file;

typedef struct {
    FILE *fd;
} wsp_io_file_inst_t;

#endif /* _WSP_IO_FILE_H_ */
