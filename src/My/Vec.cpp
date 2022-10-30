#include "My/Vec.hpp"

namespace My {
namespace Vector {
namespace Generic {

bool vec_push(Vec* vec, int typeSize, const void* elements, int elementCount) {
    const int newSize = vec->size + elementCount;
    if (newSize > vec->capacity) {
        int newCapacity = (vec->capacity * 2 > newSize) ? vec->capacity * 2 : newSize;
        void* newData = Realloc(vec->data, newCapacity * typeSize);
        if (newData) {
            vec->data = newData;
            vec->capacity = newCapacity;
        } else {
            return false;
        }
    }
    
    memcpy((char*)vec->data + vec->size * typeSize, elements, elementCount * typeSize);
    vec->size = newSize;
    return true;
}

}
}
}