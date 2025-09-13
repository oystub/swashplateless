#include "util.hpp"

#include <ctime>
#include <cstring>
#include <cstdio>

uint64_t micros64(void)
{
    static uint64_t first_us = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t tus = (uint64_t)(ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL);
    if (first_us == 0) {
        first_us = tus;
    }
    return tus - first_us;
}

uint32_t millis32(void)
{
    return (uint32_t)(micros64() / 1000ULL);
}

void getUniqueID(uint8_t id[16])
{
    memset(id, 0, 16);
    FILE* f = fopen("/etc/machine-id", "r");
    if (f) {
        fread(id, 1, 16, f);
        fclose(f);
    }
}