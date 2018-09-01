
#include "include/utils_list.h"
#include <string.h>


void *list_rpush(void *list, void* data, uint32_t * size, uint32_t each, uint32_t max)
{
    char* dst = NULL;
    if(*size < max)
    {   
        dst = (char*)list + ((*size) * each);
        memcpy(dst, data, each);
        (*size)++;
        return data;
    }
    return NULL;
}

void *list_lpush(void *list, void* data, uint32_t * size, uint32_t each, uint32_t max)
{
    int32_t i;
    if(*size < max)
    {
        for (i = 0; i < *size; i++)
        {
            memcpy((char*)list + ((*size - i - 1) * each), (char*)list + ((*size - i) * each), each);
        }
        memcpy(list, data, each);
        (*size)++;
        return data;
    }
    return NULL;
}

void *list_rpop(void *list, uint32_t * size, uint32_t each, uint32_t max)
{
    if(*size > 0)
    {
        (*size)--;
        return (char*)list + ((*size) * each);
    }
		return NULL;
}

void *list_lpop(void *list, uint32_t * size, uint32_t each, uint32_t max)
{
    int32_t i;
    uint8_t tmp[512];//guess each less than tmp size
    if(*size > 0)
    {
        memcpy(tmp, list, each);
        for (i = 1; i < *size; i++)
        {
            memcpy((char*)list + ((i - 1) * each), (char*)list + ((i) * each), each);
        }
        memcpy((char*)list + ((i + 1) * each), tmp, each);
        (*size)--;
        return (char*)list + ((i + 1) * each);
    }
		return NULL;
}


void list_remove(void *list,  uint32_t index, uint32_t each, uint32_t * size)
{
    int32_t i;
    if(index < *size)
    {
        for (i = index + 1; i < *size; i++)
        {
            memcpy((char*)list + ((i - 1) * each), (char*)list + ((i) * each), each);
        }
        (*size)--;
    }
}

