#include <pthread.h>
#include <stdio.h>
#include "Utils.hpp"

int util_cpu_bind(pthread_t tid, uint8_t mask)
{
    uint8_t i;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (i = 0; i < sizeof(mask) * 8; i++) {
        if (mask & (1 << i)) {
            CPU_SET(i, &cpuset);
        }
    }

    int rc = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
    if(rc != 0) {
        return -1;
    }
    return 0;
}
