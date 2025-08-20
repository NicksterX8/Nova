#pragma once

#include "llvm/Compiler.h"

#define ADDRESS_SANITIZER_BUILD LLVM_ADDRESS_SANITIZER_BUILD

#define LIKELY(expr) LLVM_LIKELY(expr)
#define UNLIKELY(expr) LLVM_UNLIKELY(expr)
#define UNREACHABLE LLVM_BUILTIN_UNREACHABLE
#define DEBUG_TRAP LLVM_BUILTIN_DEBUGTRAP

#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#define EMPTY_BASE_OPTIMIZE NO_UNIQUE_ADDRESS

#define PRETTY_FUNCTION LLVM_PRETTY_FUNCTION
#define LOCATION __FILE__ ":" PRETTY_FUNCTION ":" TOSTRING(__LINE__)

#ifndef NDEBUG
#define DASSERT(truthy) assert(truthy)
#else
#define DASSERT(truthy) do { if (!truthy) { __builtin_unreachable(); } } while (0)
#endif 