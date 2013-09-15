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
    WSP_MMAP = 2,
    WSP_MEMORY = 3
} wsp_mapping_t;

typedef enum {
    WSP_ERROR_NONE = 0,
    WSP_ERROR_IO = 1,
    WSP_ERROR_NOT_OPEN = 2,
    WSP_ERROR_ALREADY_OPEN = 3,
    WSP_ERROR_MALLOC = 4,
    WSP_ERROR_OFFSET = 5,
    WSP_ERROR_FUTURE_TIMESTAMP = 6,
    WSP_ERROR_RETENTION = 7,
    WSP_ERROR_ARCHIVE = 8,
    WSP_ERROR_POINT_OOB = 9,
    WSP_ERROR_UNKNOWN_AGGREGATION = 10,
    WSP_ERROR_ARCHIVE_MISALIGNED = 11,
    WSP_ERROR_TIME_INTERVAL = 12,
    WSP_ERROR_IO_MODE = 13,
    WSP_ERROR_MMAP = 14,
    WSP_ERROR_FTRUNCATE = 15,
    WSP_ERROR_FSYNC = 16,
    WSP_ERROR_OPEN = 17,
    WSP_ERROR_FOPEN = 18,
    WSP_ERROR_FILENO = 19,
    WSP_ERROR_IO_MISSING = 20,
    WSP_ERROR_IO_INVALID = 21,
    WSP_ERROR_IO_OFFSET = 22,
    WSP_ERROR_SIZE = 23
} wsp_errornum_t;

/**
 * Extra flags to set when opening a database.
 */
typedef enum {
    // open the database in write mode.
    WSP_READ = 0x01,
    // open the database in read mode.
    WSP_WRITE = 0x02
} wsp_flag_t;

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
typedef struct wsp_archive_input_t wsp_archive_input_t;
typedef struct wsp_point_input_t wsp_point_input_t;
typedef struct wsp_metadata_b wsp_metadata_b;
typedef struct wsp_metadata_t wsp_metadata_t;

typedef double wsp_value_t;

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
    int flags,
    wsp_error_t *e
);

/**
 * Create the specified whisper database using the specified mapping.
 *
 * path: Path to create the database at.
 * metadata: Metadata to use in the database.
 * archives: List of archives.
 * archives_length: Length of the list of archives.
 * e: Error object.
 */
typedef wsp_return_t (*wsp_io_create_f)(
    const char *path,
    size_t size,
    wsp_archive_t *created_archives,
    size_t count,
    wsp_metadata_t *metadata,
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
    wsp_value_t *value,
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
    wsp_io_create_f create;
} wsp_io;

struct wsp_t {
    // metadata header
    wsp_metadata_t meta;
    // specific type of mapping.
    wsp_mapping_t io_mapping;
    // io functions.
    wsp_io *io;
    // data related to a specific io instance.
    void *io_instance;
    // indicates if I/O allocates an internal buffer that needs to be
    // de-allocated after it has been used.
    int io_manual_buf;
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
    (w)->io_instance = NULL;\
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
 * flags: Open flags.
 * e: Error object.
 */
wsp_return_t wsp_open(
    wsp_t *w,
    const char *path,
    wsp_mapping_t mapping,
    int flags,
    wsp_error_t *e
);

/**
 * Create the specified whisper database using the specified mapping.
 *
 * path: Path to create the database at.
 * metadata: Metadata to use in the database.
 * archives: List of archives.
 * archives_length: Length of the list of archives.
 * mapping: The mapping to use when creating the database.
 * e: Error object.
 */
wsp_return_t wsp_create(
    const char *path,
    wsp_archive_input_t *archives,
    size_t archives_length,
    wsp_aggregation_t aggregation,
    float x_files_factor,
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
 * Same as wsp_update_now, but fetches the current timestamp from system.
 */
wsp_return_t wsp_update(
    wsp_t *w,
    wsp_point_input_t *p,
    wsp_error_t *e
);

/**
 * Insert an update in the database.
 *
 * w: Whisper database.
 * p: Point to insert.
 * now: When 'now' is.
 * e: Error object.
 */
wsp_return_t wsp_update_now(
    wsp_t *w,
    wsp_point_input_t *p,
    wsp_time_t now,
    wsp_error_t *e
);

wsp_return_t wsp_update_many(
    wsp_t *w,
    wsp_point_input_t *p,
    size_t length,
    wsp_error_t *e
);

wsp_return_t wsp_write_points(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_point_t *points,
    size_t length,
    wsp_point_t *base,
    wsp_error_t *e
);

wsp_return_t wsp_build_point(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t timestamp,
    wsp_value_t value,
    wsp_point_t *result,
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

// archive input structure.
struct wsp_archive_input_t {
    uint32_t spp;
    uint32_t count;
};

// point input structure.
struct wsp_point_input_t {
    wsp_time_t timestamp;
    wsp_value_t value;
};

#define WSP_ARCHIVE_INIT(a) do {\
    (a)->offset = 0;\
    (a)->spp = 0;\
    (a)->count = 0;\
} while(0)

wsp_return_t wsp_load_points(
    wsp_t *w,
    wsp_archive_t *archive,
    uint32_t offset,
    uint32_t size,
    wsp_point_t *result,
    wsp_error_t *e
);

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
wsp_return_t wsp_fetch_time_points(
    wsp_t *w,
    wsp_archive_t *archive,
    wsp_time_t time_from,
    wsp_time_t time_until,
    wsp_point_t *result,
    uint32_t *size,
    wsp_error_t *e
);

/**
 * Read points into buffer for a specific archive.
 * This function takes care for any wrap around in the archive.
 *
 * This function will filter points which does not match the base timestamp.
 *
 * w: Whisper database,
 * archive: Archive to load points from.
 * offset: Offset of the points to load.
 * count: Number of points to load.
 * points: Where to store points.
 * e: Error object.
 */
wsp_return_t wsp_fetch_points(
    wsp_t *w,
    wsp_archive_t *archive,
    int offset,
    uint32_t count,
    wsp_point_t *points,
    wsp_error_t *e
);

/* parse functions */
wsp_return_t wsp_parse_factor(
    const char *string,
    size_t length,
    int *result
);

wsp_return_t wsp_parse_archive_input(
    const char *string,
    wsp_archive_input_t *archive
);

wsp_return_t wsp_parse_point_input(
    const char *string,
    wsp_point_input_t *point
);

struct wsp_point_b {
    char timestamp[sizeof(uint32_t)];
    char value[sizeof(wsp_value_t)];
};

struct wsp_point_t {
    wsp_time_t timestamp;
    wsp_value_t value;
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

/*
 * Check that the io instance is of the expected type and assign it to self.
 *
 * file: The Whisper Database.
 * e: Error object.
 */
#define WSP_IO_CHECK(file, io_type, var_type, self, e) do {\
    if (file->io_instance == NULL) {\
        e->type = WSP_ERROR_IO_MISSING;\
        return WSP_ERROR;\
    }\
    if (file->io_mapping != io_type) {\
        e->type = WSP_ERROR_IO_INVALID;\
        return WSP_ERROR;\
    }\
    self = (var_type *)file->io_instance;\
} while(0)


#endif /* _WSP_H_ */
