
#include "include/utils_timer.h"


uint32_t utils_time_is_expired(iotx_time_t *timer)
{
    uint32_t cur_time;

    if (!timer) {
        return 1;
    }

    cur_time = time_get_time();
    /*
     *  WARNING: Do NOT change the following code until you know exactly what it do!
     *
     *  check whether it reach destination time or not.
     */
    if ((cur_time - timer->time) < (UINT32_MAX / 2)) {
        return 1;
    } else {
        return 0;
    }
}


uint32_t iotx_time_left(iotx_time_t *end)
{
    uint32_t now, res;

    if (!end) {
        return 0;
    }

    if (utils_time_is_expired(end)) {
        return 0;
    }

    now = time_get_time();
    res = end->time - now;
    return res;
}

uint32_t utils_time_spend(iotx_time_t *start)
{
    uint32_t now, res;

    if (!start) {
        return 0;
    }

    now = time_get_time();
    res = now - start->time;
    return res;
}


void utils_time_countdown_ms(iotx_time_t *timer, uint32_t millisecond)
{
    if (!timer) {
        return;
    }

    timer->time = time_get_time() + millisecond;
}

