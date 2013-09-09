#ifndef _WSP_DEBUG_H_
#define _WSP_DEBUG_H_

#define TO_BOOL(expr) \
    ((expr) ? "true" : "false")

#define DEBUG_PRINTF(...) \
    do { \
        fprintf(stdout, __VA_ARGS__); \
    } while (0);

#endif /* _WSP_DEBUG_H */
