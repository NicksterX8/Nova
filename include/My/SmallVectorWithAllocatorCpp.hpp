//===- llvm/ADT/SmallVector.cpp - 'Normally small' vectors ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the SmallVector class.
//
//===----------------------------------------------------------------------===//

#include "SmallVectorWithAllocator.hpp"
// #include "llvm/ADT/Twine.h"
// #include "llvm/Support/MemAlloc.h"
#include <cstdint>

// Check that no bytes are wasted and everything is well-aligned.
namespace {
// These structures may cause binary compat warnings on AIX. Suppress the
// warning since we are only using these types for the static assertions below.
#if defined(_AIX)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waix-compat"
#endif
struct Struct16B {
  alignas(16) void *X;
};
struct Struct32B {
  alignas(32) void *X;
};
#if defined(_AIX)
#pragma GCC diagnostic pop
#endif
}

/*
// Static asserts broke vscode for some reason
static_assert(sizeof(SmallVector<void *, 0>) ==
                  sizeof(unsigned) * 2 + sizeof(void *),
              "wasted space in SmallVector size 0");
static_assert(alignof(SmallVector<Struct16B, 0>) >= alignof(Struct16B),
              "wrong alignment for 16-byte aligned T");
static_assert(alignof(SmallVector<Struct32B, 0>) >= alignof(Struct32B),
              "wrong alignment for 32-byte aligned T");
static_assert(sizeof(SmallVector<Struct16B, 0>) >= alignof(Struct16B),
              "missing padding for 16-byte aligned T");
static_assert(sizeof(SmallVector<Struct32B, 0>) >= alignof(Struct32B),
              "missing padding for 32-byte aligned T");
static_assert(sizeof(SmallVector<void *, 1>) ==
                  sizeof(unsigned) * 2 + sizeof(void *) * 2,
              "wasted space in SmallVector size 1");

static_assert(sizeof(SmallVector<char, 0>) ==
                  sizeof(void *) * 2 + sizeof(void *),
              "1 byte elements have word-sized type for size and capacity");
*/

namespace detail {

// Note: Moving this function into the header may cause performance regression.
template <class Size_T>
static size_t getNewCapacity(size_t MinSize, size_t TSize, size_t OldCapacity) {
  constexpr size_t MaxSize = std::numeric_limits<Size_T>::max();

  // Ensure we can fit the new capacity.
  // This is only going to be applicable when the capacity is 32 bit.
  // if (MinSize > MaxSize)
  //   report_size_overflow(MinSize, MaxSize);

  // Ensure we can meet the guarantee of space for at least one more element.
  // The above check alone will not catch the case where grow is called with a
  // default MinSize of 0, but the current capacity cannot be increased.
  // This is only going to be applicable when the capacity is 32 bit.
  // if (OldCapacity == MaxSize)
  //   report_at_maximum_capacity(MaxSize);

  // In theory 2*capacity can overflow if the capacity is 64 bit, but the
  // original capacity would never be large enough for this to be a problem.
  size_t NewCapacity = 2 * OldCapacity + 1; // Always grow.
  return std::min(std::max(NewCapacity, MinSize), MaxSize);
}

}

template <class Size_T, typename AllocatorT>
void *llvm::WithAllocator::SmallVectorBase<Size_T, AllocatorT>::replaceAllocation(void *NewElts, size_t TSize,
                                                 size_t OldCapacity, size_t NewCapacity,
                                                 size_t VSize) {
  void *NewEltsReplace = getAllocator().allocate(NewCapacity * TSize, alignof(std::max_align_t));
  if (VSize)
    memcpy(NewEltsReplace, NewElts, VSize * TSize);
  getAllocator().deallocate(NewElts, OldCapacity * TSize, alignof(std::max_align_t));
  return NewEltsReplace;
}

// Note: Moving this function into the header may cause performance regression.
template <class Size_T, typename AllocatorT>
void *llvm::WithAllocator::SmallVectorBase<Size_T, AllocatorT>::mallocForGrow(void *FirstEl, size_t MinSize,
                                             size_t TSize,
                                             size_t &NewCapacity) {
  NewCapacity = ::detail::getNewCapacity<Size_T>(MinSize, TSize, this->capacity());
  // Even if capacity is not 0 now, if the vector was originally created with
  // capacity 0, it's possible for the malloc to return FirstEl.
  void *NewElts = getAllocator().allocate(NewCapacity * TSize, alignof(std::max_align_t));
  if (NewElts == FirstEl)
    NewElts = replaceAllocation(NewElts, TSize, Capacity, NewCapacity);
  return NewElts;
}

// Note: Moving this function into the header may cause performance regression.
template <class Size_T, typename AllocatorT>
void llvm::WithAllocator::SmallVectorBase<Size_T, AllocatorT>::grow_pod(void *FirstEl, size_t MinSize,
                                       size_t TSize) {
  size_t NewCapacity = ::detail::getNewCapacity<Size_T>(MinSize, TSize, this->capacity());
  void *NewElts;
  if (BeginX == FirstEl) {
    NewElts = getAllocator().allocate(NewCapacity * TSize, alignof(std::max_align_t));
    if (NewElts == FirstEl)
      NewElts = replaceAllocation(NewElts, TSize, Capacity, NewCapacity);

    // Copy the elements over.  No need to run dtors on PODs.
    memcpy(NewElts, this->BeginX, size() * TSize);
  } else {
    // If this wasn't grown from the inline copy, grow the allocated space.
    NewElts = getAllocator().reallocate(this->BeginX, this->Capacity * TSize, NewCapacity * TSize, alignof(std::max_align_t));
    if (NewElts == FirstEl)
      NewElts = replaceAllocation(NewElts, TSize, Capacity, NewCapacity, size());
  }

  this->BeginX = NewElts;
  this->Capacity = NewCapacity;
}

// template class llvm::WithAllocator::SmallVectorBase<uint32_t, Mallocator>;

// Disable the uint64_t instantiation for 32-bit builds.
// Both uint32_t and uint64_t instantiations are needed for 64-bit builds.
// This instantiation will never be used in 32-bit builds, and will cause
// warnings when sizeof(Size_T) > sizeof(size_t).
#if SIZE_MAX > UINT32_MAX
// template class llvm::WithAllocator::SmallVectorBase<uint64_t, Mallocator>;


#endif