// vim: foldmethod=marker
/**
 * Whisper Database Functions.
 *
 * Read the 'Error Handling' section since it details the most important aspect
 * of using this library.
 *
 * Opening a Database
 * ------------------
 * wsp_open
 *
 * Example:
 *
 *   if (wsp_open(&w, p, WSP_MMAP, &e) == WSP_ERROR) {
 *       printf("%s: %s: %s\n", wsp_strerror(&e), strerror(e.syserr), p);
 *       return 1;
 *   }
 *
 * Error Handling
 * --------------
 * The details of error handling of each function will not be documented unless
 * there is an exception to any of these rules.
 *
 * - All functions return WSP_OK on success or WSP_ERROR on failure.
 * - All functions take an wsp_error_t as the last argument.
 * - On failure, the last error object will be populated with details about the
 *   encountered problem.
 * - On failure, all functions are doing their best to be atomic, failures
 *   should not cause undefined state.
 */
#ifndef _WSP_H_
#define _WSP_H_

/**
 * If defined, archives will be validated when they are read.
 */
#define VALIDATE_ARCHIVE

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#include "wsp_time.h"

struct wsp_error_t;
struct wsp_t;
struct wsp_point_b;
struct wsp_point_t;
struct wsp_archive_b;
struct wsp_archive_t;
struct wsp_metadata_b;
struct wsp_metadata_t;

typedef enum {
    WSP_ERROR = -1,
    WSP_OK = 0
} wsp_return_t;

typedef enum {
    WSP_MAPPING_NONE = 0,
    WSP_FILE = 1,
    WSP_MMAP = 2
} wsp_mapping_t;

typedef enum {
    WSP_ERROR_NONE = 0,
    WSP_ERROR_NOT_INITIALIZED = 1,
    WSP_ERROR_ALREADY_INITIALIZED = 2,
    WSP_ERROR_IO = 3,
    WSP_ERROR_NOT_OPEN = 4,
    WSP_ERROR_ALREADY_OPEN = 5,
    WSP_ERROR_MALLOC = 6,
    WSP_ERROR_OFFSET = 7,
    WSP_ERROR_FUTURE_TIMESTAMP = 8,
    WSP_ERROR_RETENTION = 9,
    WSP_ERROR_ARCHIVE = 10,
    WSP_ERROR_POINT_OOB = 11,
    WSP_ERROR_UNKNOWN_AGGREGATION = 12,
    WSP_ERROR_ARCHIVE_MISALIGNED = 13,
    WSP_ERROR_TIME_INTERVAL = 14,
    WSP_ERROR_SIZE = 15
} wsp_errornum_t;

typedef enum {
    WSP_AVERAGE = 1,
    WSP_SUM = 2,
    WSP_LAST = 3,
    WSP_MAX = 4,
    WSP_MIN = 5
} wsp_aggregation_t;

typedef struct wsp_error_t wsp_error_t;
typedef struct wsp_t wsp_t;
typedef struct wsp_point_b wsp_point_b;
typedef struct wsp_point_t wsp_point_t;
typedef struct wsp_archive_b wsp_archive_b;
typedef struct wsp_archive_t wsp_archive_t;
typedef struct wsp_metadata_b wsp_metadata_b;
typedef struct wsp_metadata_t wsp_metadata_t;

const char *wsp_strerror(wsp_error_t *);

struct wsp_error_t {
    wsp_errornum_t type;
    int syserr;
};

#define WSP_ERROR_INIT(e) do {\
    (e)->type = WSP_ERROR_NONE;\
    (e)->syserr = 0;\
} while(0)

/**
 * I/O mapping reader function that allocates memory when required.
 *
 * After using this, the w->io_manual_buf flag should be checked.
 *
 * w: Whisper database.
 * offset: Offset to read.
 * size: Size to read.
 * buf: Reference for buffer to read into, if points addressed is NULL, space
 * will be allocated that might have to be freed depending on the value of
 * w->io_manual_buf.
 * e: Error object.
 */
typedef wsp_return_t(*wsp_io_read_f)(
    wsp_t *w,
    long offset,
    size_t size,
    void **buf,
    wsp_error_t *e
);

/**
 * I/O mapping reader function that does not allocate memory when required.
 *
 * w->io_manual_buf does not have to be checked since it is guaranteed to read
 * directly into the specified buffer.
 *
 * w: Whisper database.
 * offset: Offset to read.
 * size: Size to read.
 * buf: Buffer to read into.
 * e: Error object.
 */
typedef wsp_return_t(*wsp_io_read_into_f)(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
);

/**
 * I/O mapping writer function.
 *
 * w: Whisper database.
 * offset: Offset to write.
 * size: Size to write.
 * buf: Buffer to write from.
 * e: Error object.
 */
typedef wsp_return_t(*wsp_io_write_f)(
    wsp_t *w,
    long offset,
    size_t size,
    void *buf,
    wsp_error_t *e
);

/**
 * I/O mapping open function.
 *
 * w: Whisper database.
 * path: Path to open.
 * e: Error object.
 */
typedef wsp_return_t(*wsp_io_open_f)(
    wsp_t *w,
    const char *path,
    wsp_error_t *e
);

/**
 * I/O mapping close function.
 *
 * w: Whisper database.
 * e: Error object.
 */
typedef wsp_return_t(*wsp_io_close_f)(
    wsp_t *w,
    wsp_error_t *e
);

/**
 * Whisper aggregate function.
 *
 * Most of the available functions are defined in wsp_private.c.
 *
 * w: Whisper database.
 * points: Points to aggregate.
 * count : Amount of points that are being aggregated.
 * value: Where to store the resulting value.
 * skip: Will be changed to 1 if there are not enough points to aggregate
 * a value, and the rest of the archives should be skipped.
 * e: Error object.
 */
typedef wsp_return_t(*wsp_aggregate_f)(
    wsp_t *w,
    wsp_point_t *points,
    uint32_t count,
    double *value,
    int *skip,
    wsp_error_t *e
);

struct wsp_metadata_b {
    char aggregation[sizeof(uint32_t)];
    char max_retention[sizeof(uint32_t)];
    char x_files_factor[sizeof(float)];
    char archives_count[sizeof(uint32_t)];
};

struct wsp_metadata_t {
    wsp_aggregation_t aggregation;
    uint32_t max_retention;
    float x_files_factor;
    uint32_t archives_count;
    // aggregate function.
    wsp_aggregate_f aggregate;
};

#define WSP_METADATA_INIT(m) do {\
    (m)->aggregation = 0;\
    (m)->max_retention = 0;\
    (m)->x_files_factor = 0;\
    (m)->archives_count = 0;\
    (m)->aggregate = NULL;\
} while(0)

typedef struct {
    wsp_io_open_f open;
    wsp_io_close_f close;
    wsp_io_read_f read;
    wsp_io_read_into_f read_into;
    wsp_io_write_f write;
} wsp_io;

struct wsp_t {
    // metadata header
    wsp_metadata_t meta;
    // file descriptor (as returned by fopen)
    FILE *io_fd;
    // mapped memory of file.
    void *io_mmap;
    // size of the file, for later munmap call.
    off_t io_size;
    // specific type of mapping.
    wsp_mapping_t io_mapping;
    // indicates if I/O allocates an internal buffer that needs to be
    // de-allocated after it has been used.
    int io_manual_buf;
    // io functions.
    wsp_io *io;
    // archives
    // these are empty (NULL) until wsp_load_archives has been called.
    wsp_archive_t *archives;
    // Size in bytes of the archive block in the database.
    size_t archives_size;
    // Real archive count that has *actually* been loaded.
    // This might differ from metadata if laoding fails.
    uint32_t archives_count;
};

#define WSP_INIT(w) do {\
    (w)->io_fd = NULL;\
    (w)->io_mmap = NULL;\
    (w)->io_size = 0;\
    (w)->io_mapping = 0;\
    (w)->io_manual_buf = 0;\
    (w)->io = NULL;\
    (w)->archives = NULL;\
    (w)->archives_size = 0;\
    (w)->archives_count = 0;\
} while(0)

/**
 * Open the specified path as a whisper database.
 *
 * w: Whisper database handle, should have been initialized using WSP_INIT
 * prior to this function.
 * path: Path to the file containing the whisper database.
 * mapping: The file mapping method to use; WSP_MMAP or WSP_FILE.
 * e: Error object.
 */
wsp_return_t wsp_open(
    wsp_t *w,
    const char *path,
    wsp_mapping_t mapping,
    wsp_error_t *e
);

/**
 * Close an already open whisper database.
 *
 * w: Whisper database.
 * e: Error object.
 */
wsp_return_t wsp_close(
    wsp_t *w,
    wsp_error_t *e
);

/**
 * Insert an update in the database.
 *
 * w: Whisper database.
 * p: Point to insert.
 * e: Error object.
 */
wsp_return_t wsp_update(
    wsp_t *w,
    wsp_point_t *p,
    wsp_error_t *e
);

wsp_return_t wsp_update_point(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t timestamp,
    double value,
    wsp_point_t *base,
    wsp_error_t *e
);

struct wsp_archive_b {
    char offset[sizeof(uint32_t)];
    char spp[sizeof(uint32_t)];
    char count[sizeof(uint32_t)];
};

struct wsp_archive_t {
    // absolute offset of archive in database.
    uint32_t offset;
    // seconds per point.
    uint32_t spp;
    // the amount of points in database.
    uint32_t count;
    /* extra fields */
    size_t points_size;
    uint64_t retention;
};

/**
 * Load points between two timestamps.
 *
 * w: Whisper Database
 * archive: Whisper archive to load from.
 * time_from: Start of time interval.
 * time_until: End of time interval.
 * result: Where to store the result, this should have at least archive->count
 * space already allocated.
 * size: Where to store the length of the resulting points.
 * e: Error object.
 */
wsp_return_t wsp_load_time_points(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t time_from,
    wsp_time_t time_until,
    wsp_point_t *result,
    uint32_t *size,
    wsp_error_t *e
);

/**
 * Like wsp_load_points but will read all points.
 */
wsp_return_t wsp_load_all_points(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_point_t *points,
    wsp_error_t *e
);

/**
 * Read points into buffer for a specific archive.
 * This function takes care for any wrap around in the archive.
 *
 * w: Whisper database,
 * archive: Archive to load points from.
 * offset: Offset of the points to load.
 * count: Number of points to load.
 * points: Where to store points.
 * e: Error object.
 */
wsp_return_t wsp_load_points(
    wsp_t *w,
    wsp_archive_t *archive,
    int offset,
    uint32_t count,
    wsp_point_t *points,
    wsp_error_t *e
);

struct wsp_point_b {
    char timestamp[sizeof(uint32_t)];
    char value[sizeof(double)];
};

struct wsp_point_t {
    wsp_time_t timestamp;
    double value;
};

#define WSP_POINT_INIT(p) do { \
    (p)->timestamp = 0; \
    (p)->value = 0; \
} while(0)

/*
 * Calculate the absolute offset for a specific point in the database.
 */
#define WSP_POINT_OFFSET(archive, index) \
    ((archive)->offset + sizeof(wsp_point_b) * index)

#define WSP_ARCHIVE_OFFSET(index) \
    (sizeof(wsp_metadata_b) + sizeof(wsp_archive_b) * index)

#endif /* _WSP_H_ */
