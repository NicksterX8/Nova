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

#define LIKELY(expr) LLVM_LIKELY(expr)
#define UNLIKELY(expr) LLVM_UNLIKELY(expr)

#define FIRST(first, ...) first
#define ALL_AFTER_FIRST(first, ...) __VA_ARGS__

#define DO_ONCE(function) static int COMBINE(_do_once_dummy_var, __LINE__)[] = {0, (function, 0)}

#define PRETTY_FUNCTION LLVM_PRETTY_FUNCTION
#define LOCATION __FILE__ ":" PRETTY_FUNCTION ":" TOSTRING(__LINE__)

#endif