#pragma once

#include "ADT/SmallVector.hpp"

template<typename T, typename AllocatorT = Mallocator, unsigned N = SmallVectorDefaultInlinedElements<T, AllocatorT>>
struct UnorderedTwoSidedVector {
    SmallVectorA<T, AllocatorT, 0> vec;
    int leftEnd = 0;

    // using Base = SmallVectorA<T, AllocatorT, 0>;

    using size_type = uint32_t;
    using index_type = uint32_t;

    size_type leftSize() const {
        return leftEnd;
    }

    size_type rightSize() const {
        return vec.size() - leftEnd;
    }

    T* leftData() const {
        return vec.data();
    }

    T* rightData() const {
        return vec.data() + leftEnd;
    }

    index_type push_left(const T& elt, ) {
        T* end = vec.require(1);
        if (rightSize() > 0) {
            // swap first right and new last right. 
            T* firstRight = &rightData()[0];
            *end = *firstRight;
            // first right now opened up for new left element. then we claim the space for left with leftEnd++
            *firstRight = elt;
        } else {
            *end = elt;
        }
        return leftEnd++;
    }

    index_type push_right(const T& elt) {
        vec.push_back(elt);
        return vec.size()-1;
    }

    void erase_left(index_type index) {
        assert(index < leftEnd && "tried to erase_left index that isn't left side")
    }

    void erase(index_type index) {
        if (index < leftEnd) {
            // left

        } else {
            // right

        }
    }

};

