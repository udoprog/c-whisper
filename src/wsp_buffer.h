/**
 * Internal structures used to account for endianness.
 */
#ifndef _WSP_BUFFER_H_
#define _WSP_BUFFER_H_

#include "wsp.h"

struct wsp_metadata_b;
typedef struct wsp_metadata_b wsp_metadata_b;

struct wsp_metadata_b {
    char aggregation[sizeof(uint32_t)];
    char max_retention[sizeof(uint32_t)];
    char x_files_factor[sizeof(float)];
    char archives_count[sizeof(uint32_t)];
};

void __wsp_parse_metadata(
    wsp_metadata_b *buf,
    wsp_metadata_t *m
);

void __wsp_dump_metadata(
    wsp_metadata_t *m,
    wsp_metadata_b *buf
);

struct wsp_archive_b;
typedef struct wsp_archive_b wsp_archive_b;

#define WSP_ARCHIVE_OFFSET(index) \
    (sizeof(wsp_metadata_b) + sizeof(wsp_archive_b) * index)

struct wsp_archive_b {
    char offset[sizeof(uint32_t)];
    char spp[sizeof(uint32_t)];
    char count[sizeof(uint32_t)];
};

void __wsp_dump_archives(
    wsp_archive_t *archives,
    uint32_t count,
    wsp_archive_b *buf
);

void __wsp_parse_archive(
    wsp_archive_b *buf,
    wsp_archive_t *archive
);

void __wsp_dump_archive(
    wsp_archive_t *archive,
    wsp_archive_b *buf
);

struct wsp_point_b;
typedef struct wsp_point_b wsp_point_b;

/**
 * Calculate the absolute offset for a specific point in the database.
 */
#define WSP_POINT_OFFSET(archive, index) \
    ((archive)->offset + sizeof(wsp_point_b) * index)

struct wsp_point_b {
    char timestamp[sizeof(uint32_t)];
    char value[sizeof(wsp_value_t)];
};

void __wsp_parse_point(
    wsp_point_b *buf,
    wsp_point_t *p
);

void __wsp_dump_point(
    wsp_point_t *p,
    wsp_point_b *buf
);

#endif /* _WSP_BUFFER_H_ */
