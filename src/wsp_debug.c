#include <stdarg.h>
#include <stdio.h>

#include "wsp_debug.h"

void debug_printf(
    const char *file,
    int line,
    const char *function,
    const char *format,
    ...
)
{
    char buffer[1024];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);

    fprintf(stdout, "%s:%d (%s): %s\n", file, line, function, buffer);
}
