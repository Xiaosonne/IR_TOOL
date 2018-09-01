
#ifndef _IOTX_COMMON_LIST_H
#define _IOTX_COMMON_LIST_H

#include <stdlib.h>
#include <stdint.h>


void *list_rpush(void *list, void* data, uint32_t *size, uint32_t each, uint32_t max);

void *list_lpush(void *list, void* data, uint32_t *size, uint32_t each, uint32_t max);

void *list_rpop(void *list, uint32_t *size, uint32_t each, uint32_t max);

void *list_lpop(void *list, uint32_t *size, uint32_t each, uint32_t max);

void list_remove(void *list,  uint32_t index, uint32_t each, uint32_t *size);

#endif

