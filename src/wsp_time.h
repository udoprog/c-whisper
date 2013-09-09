// vim: foldmethod=marker
/**
 * Time related functions.
 *
 * The general directions is to not use a type like time_t directly.
 * If it is realted to an absolute point in time, use these.
 */
#ifndef _WSP_TIME_H_
#define _WSP_TIME_H_

#include <stdint.h>

typedef uint32_t wsp_time_t;

wsp_time_t wsp_time_now(void);
wsp_time_t wsp_time_floor(wsp_time_t base, wsp_time_t interval);
wsp_time_t wsp_time_from_timestamp(uint32_t timestamp);

#endif /* _WSP_TIME_H_ */
