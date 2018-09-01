

#include <stdint.h>

#ifndef _IOTX_COMMON_TIMER_H_
#define _IOTX_COMMON_TIMER_H_


typedef struct {
    uint32_t time;
} iotx_time_t;


extern uint32_t time_get_time(void);

uint32_t utils_time_is_expired(iotx_time_t *timer);

uint32_t iotx_time_left(iotx_time_t *end);

uint32_t utils_time_spend(iotx_time_t *start);

void utils_time_countdown_ms(iotx_time_t *timer, uint32_t millisecond);


#endif /* _IOTX_COMMON_TIMER_H_ */
