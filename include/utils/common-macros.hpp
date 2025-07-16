#ifndef UTILS_COMMON_MACROS_INCLUDED
#define UTILS_COMMON_MACROS_INCLUDED

#include "defer.hpp"
#include "llvm/Compiler.h"

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define COMMA ,

#define IABS(integer) ((integer) >= 0 ? (integer) : -(integer))

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

#define FOR_EACH_VAR_TYPE(func_call) int COMBINE(_dummy_for_each_var_type_helper, __LINE__)[] = {0, (func_call, 0) ...}; (void)COMBINE(_dummy_for_each_var_type_helper, __LINE__);

#define ssizeof(v) ((ssize_t)sizeof(v)) 

#define FIRST(first, ...) first
#define ALL_AFTER_FIRST(first, ...) __VA_ARGS__

#define DO_ONCE(function) static int COMBINE(_do_once_dummy_var, __LINE__)[] = {0, (function, 0)}

// M_GET_LAST

#define M_GET_ELEM(N, ...) COMBINE(M_GET_ELEM_, N)(__VA_ARGS__)
#define M_GET_ELEM_0(_0, ...) _0
#define M_GET_ELEM_1(_0, _1, ...) _1
#define M_GET_ELEM_2(_0, _1, _2, ...) _2
#define M_GET_ELEM_3(_0, _1, _2, _3, ...) _3
#define M_GET_ELEM_4(_0, _1, _2, _3, _4, ...) _4
#define M_GET_ELEM_5(_0, _1, _2, _3, _4, _5, ...) _5
#define M_GET_ELEM_6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define M_GET_ELEM_7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define M_GET_ELEM_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define M_GET_ELEM_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define M_GET_ELEM_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10

// Get last argument - placeholder decrements by one
#define M_GET_LAST(...) M_GET_ELEM(M_NARGS(__VA_ARGS__), _, __VA_ARGS__ ,,,,,,,,,,,)

#endif