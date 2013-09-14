#include <check.h>

#include "check_utils.h"

#include "../src/wsp.h"
#include "../src/wsp_io_memory.h"

/*
 * Check that a specific file does not exist.
 */
void wsp_ck_not_file(
    const char *name
)
{
    ck_assert(wsp_memfs_find(&memfs_ctx, name) == NULL);
}

/*
 * Check that a specific file does exist and have the expected number of
 * size.
 */
void wsp_ck_file_size(
    const char *name,
    wsp_archive_input_t *archives,
    size_t archive_count
)
{
    wsp_memfs_t *mf = wsp_memfs_find(&memfs_ctx, name);

    size_t expected_size = (
        sizeof(wsp_metadata_b) + sizeof(wsp_archive_b) * archive_count
    );

    uint32_t i;

    for (i = 0; i < archive_count; i++) {
        expected_size += archives[i].count * sizeof(wsp_point_b);
    }

    ck_assert(mf != NULL);
    ck_assert_int_eq(expected_size, mf->size);
}

