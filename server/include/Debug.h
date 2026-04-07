#pragma once
// #define DEBUG_MEMCPY
#define ASSERT_TYPE_EQ(T1, T2)                                                                     \
    static_assert(_Generic(((T1){ 0 }), T2: 1, default: 0), "Mismatched types.")
