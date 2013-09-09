#include "wsp_time.h"

#include <time.h>

wsp_time_t wsp_time_now(void)
{
    return time(NULL);
}

wsp_time_t wsp_time_floor(wsp_time_t base, wsp_time_t interval)
{
    return base - (base % interval);
}

wsp_time_t wsp_time_from_timestamp(wsp_time_t timestamp)
{
    return (wsp_time_t)timestamp;
}
