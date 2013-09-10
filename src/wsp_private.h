// vim: foldmethod=marker
/**
 * Private functions not intended for external use.
 */
#ifndef _WSP_PRIVATE_H_
#define _WSP_PRIVATE_H_

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

/**
 * Filter out points depending on a base values timestamp.
 *
 * This makes sure that invalid values will be set to NaN.
 */
wsp_return_t __wsp_filter_points(
    wsp_point_t *base,
    wsp_archive_t *archive,
    int offset,
    uint32_t count,
    wsp_point_t *points,
    wsp_point_t *result,
    wsp_error_t *e
);

wsp_return_t __wsp_fetch_read_points(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t from,
    uint32_t until,
    uint32_t count,
    wsp_point_t *points,
    wsp_error_t *e
);

wsp_io *__wsp_get_io(wsp_mapping_t mapping);

uint32_t __wsp_point_mod(int value, uint32_t div);

void __wsp_parse_points(
    wsp_point_b *buf,
    uint32_t count,
    wsp_point_t *points
);

void __wsp_dump_metadata(
    wsp_metadata_t *m,
    wsp_metadata_b *buf
);

void __wsp_dump_points(
    wsp_point_t *points,
    uint32_t count,
    wsp_point_b *buf
);

void __wsp_dump_archives(
    wsp_archive_t *archives,
    uint32_t count,
    wsp_archive_b *buf
);

#endif /* _WSP_PRIVATE_H_ */
