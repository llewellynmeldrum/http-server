#pragma once

#define NOP ((void)0)
#define CONCAT(a, b) a##b

#define LBRACE (
#define RBRACE )

// clang-format off
#define MACRO_BEGIN do{
#define MACRO_END }while (0)
// clang-format on
