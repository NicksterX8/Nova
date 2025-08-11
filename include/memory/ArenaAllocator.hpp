#pragma once

#include "llvm/Allocator.h"
#include "Allocator.hpp"

using ArenaAllocator = llvm::BumpPtrAllocatorImpl<Mallocator, 1024>;
using LargeArenaAllocator = llvm::BumpPtrAllocatorImpl<Mallocator, 4096>;