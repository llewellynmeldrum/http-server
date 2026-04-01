#pragma once
#include <time.h>

#include "CWrappers.h"
extern u64 program_epoch_ns;

static inline double nstoms(const u64 ns) {
    return ns / 1000000.0;
}
static inline double stons(const u64 ns) {
    return ns * 1000000000ULL;
}

static inline double get_current_ns(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (u64)stons(ts.tv_sec) + ts.tv_nsec;
}

static inline u64 ms_since_start(void) {
    u64    current_ns = get_current_ns();
    u64    ns_elapsed = current_ns - program_epoch_ns;
    double ms = nstoms(ns_elapsed);
    return ms;
}
