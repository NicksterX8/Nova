

#include "MyInternals.hpp"
#include "Array.hpp"
#include "../utils/common-macros.hpp"

MY_CLASS_START

Array<void> reallocate(void* data, int size, int newCapacity) {
    newCapacity = (size > newCapacity) ? size : newCapacity; // new capacity can't be less than size

    if (newCapacity != 0) {
        void* newData = MY_realloc(data, newCapacity);
        if (!newData) {
            return -1;
        }
        data = newData;
    } else {
        MY_free(data);
        data = nullptr;
    }
    
    return Array<void>(newCapacity, data);
}

MY_CLASS_END