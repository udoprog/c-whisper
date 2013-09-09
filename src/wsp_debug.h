#ifndef _WSP_DEBUG_H_
#define _WSP_DEBUG_H_

#if defined(WSP_DEBUG)
#    define DEBUG (1)
#else
#    define DEBUG (0)
#endif

#define to_bool(expr) \
    ((expr) ? "true" : "false")

#define DEBUG_PRINTF(...) \
    debug_printf(__FILE__, __LINE__, __func__, __VA_ARGS__)

void debug_printf(
    const char *file,
    int line,
    const char *function,
    const char *format,
    ...
);

#endif /* _WSP_DEBUG_H_ */
