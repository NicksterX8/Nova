#pragma once

#include "llvm/PointerUnion.h"

struct TinyStr {
    using Unowned = const char*;
    using Owned = char*;
    llvm::PointerUnion<Unowned, Owned> ptr;

    TinyStr() {}
};