#pragma once

#include "llvm/ArrayRef.h"

using llvm::ArrayRef;
using llvm::MutableArrayRef;

template<typename T>
using MutArrayRef = llvm::MutableArrayRef<T>;
#define ARRAY_REF(ptr, size) ArrayRef<std::remove_reference_t<decltype(*ptr)>::type>(ptr, size)
#define MUT_ARRAY_REF(ptr, size) MutableArrayRef<std::remove_reference_t<decltype(*ptr)>::type>(ptr, size)