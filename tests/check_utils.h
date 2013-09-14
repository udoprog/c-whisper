#ifndef _CHECK_UTILS_H_
#define _CHECK_UTILS_H_

#include "../src/wsp.h"

void wsp_ck_not_file(
    const char *name
);

void wsp_ck_file_size(
    const char *name,
    wsp_archive_input_t *archives,
    size_t archive_count
);

#endif /* _CHECK_UTILS_H_ */
