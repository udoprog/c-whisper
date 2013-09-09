// vim: foldmethod=marker
/**
 * Private functions not intended for external use.
 */
#ifndef _WSP_PRIVATE_H_
#define _WSP_PRIVATE_H_

/*
 * Setup memory mapping for the specified file.
 *
 * io_fd: The file descriptor to memory map.
 * io_mmap: Pointer to a pointer that will be written to the new memory region.
 * io_size: Pointer to write the file size to.
 * e: Error object.
 */
wsp_return_t __wsp_setup_mmap(
    FILE *io_fd,
    void **io_mmap,
    off_t *io_size,
    wsp_error_t *e
);

/*
 * Read metadata from file.
 *
 * w: Whisper file to read metadata from.
 * m: Pointer to data that will be updated with the read metadata.
 * e: Error object.
 */
wsp_return_t __wsp_read_metadata(
    wsp_t *w,
    wsp_metadata_t *m,
    wsp_error_t *e
);

wsp_return_t __wsp_read_archive(
    wsp_t *w,
    int index,
    wsp_archive_t *archive,
    wsp_error_t *e
);

wsp_return_t __wsp_valid_archive(
    wsp_archive_t *prev,
    wsp_archive_t *cur,
    wsp_error_t *e
);

wsp_return_t __wsp_load_archives(
    wsp_t *w,
    wsp_error_t *e
);

wsp_return_t __wsp_archive_free(
    wsp_archive_t *archive,
    wsp_error_t *e
);

wsp_return_t __wsp_find_highest_precision(
    wsp_time_t diff,
    wsp_t *w,
    wsp_archive_t **low,
    uint32_t *low_size,
    wsp_error_t *e
);

wsp_return_t __wsp_save_point(
    wsp_t *w,
    wsp_archive_t *archive,
    long index,
    wsp_point_t *point,
    wsp_error_t *e
);

/**
 * Load a single point from an archive.
 *
 * w: Whisper database.
 * archive: The archive to read the point from.
 * index: Index of the point to read.
 * point: Where to store the read point.
 * e: Error object.
 */
wsp_return_t __wsp_load_point(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t index,
    wsp_point_t *point,
    wsp_error_t *e
);

int __wsp_load_points(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t offset,
    uint32_t size,
    wsp_point_t *result,
    wsp_error_t *e
);

uint32_t __wsp_point_mod(int value, uint32_t div);

#endif /* _WSP_PRIVATE_H_ */
