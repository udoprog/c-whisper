#ifndef _WSP_DEBUG_H_
#define _WSP_DEBUG_H_

#if defined(WSP_DEBUG)
#    define DEBUG (1)
#else
#    define DEBUG (0)
#endif

#define TO_BOOL(expr) \
    ((expr) ? "true" : "false")

#define DEBUG_PRINTF(...) \
    do { \
        fprintf(stdout, __VA_ARGS__); \
    } while (0);

#endif /* _WSP_DEBUG_H */
