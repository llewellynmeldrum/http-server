#pragma once
#include <SDL3/SDL.h>
#include <stdint.h>

typedef const SDL_FColor* ColorScheme;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

// clang-format off -> really doesnt like xmacros
#define Direction__LIST                                                                            \
    X(Direction_UP)                                                                                \
    X(Direction_LEFT)                                                                              \
    X(Direction_DOWN)                                                                              \
    X(Direction_RIGHT)

#define X(dir) dir,
typedef enum Direction {
    Direction__LIST  //
        Direction_COUNT
} Direction;
#undef X
const char* Direction_tostr(Direction dir);
