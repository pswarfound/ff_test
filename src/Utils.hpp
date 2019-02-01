#pragma once
#include <stdint.h>
#include <pthread.h>

int util_cpu_bind(pthread_t tid, uint8_t mask);

static inline uint32_t util_diff_time_us(struct timeval &t1, struct timeval &t2)
{
    return (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec - t1.tv_usec;
}
