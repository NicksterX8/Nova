#ifndef TINY_VAL_VECTOR_INCLUDED
#define TINY_VAL_VECTOR_INCLUDED

#include "llvm/PointerUnion.h"
#include "ValPtrUnion.hpp"
#include "utils/systemInfo.hpp"

template<typename T, uint8_t UnusedBitIndex, bool UnusedBitState = false, bool AssertBitUnused = false>
class TinyValVectorFull : ValPtrUnion<T, llvm::SmallVector<T, 0>*, UnusedBitIndex, UnusedBitState, AssertBitUnused> {
    using VectorT = llvm::SmallVector<T, 0>;
    using Base = ValPtrUnion<T, llvm::SmallVector<T, 0>*, UnusedBitIndex, UnusedBitState, AssertBitUnused>;
    using Me = TinyValVectorFull<T, UnusedBitIndex, UnusedBitState>;

public:
    TinyValVectorFull() : Base((VectorT*)nullptr) {}

    TinyValVectorFull(T value) : Base(value) {

    }

    TinyValVectorFull(ArrayRef<T> array) {
        if (array.size() == 1) {
            Base::setValue(array[0]);
        } else {
            VectorT* vector = newVector({array.begin(), array.end()});
            Base::setPointer(vector);
        }
    }

    TinyValVectorFull(const Me& copy) : Base(copy) {
        if (copy.Base::isPointer()) {
            auto* vector = copy.Base::getPointer();
            if (vector) {
                auto* copiedVector = newVector(*vector);
                Base::setPointer(copiedVector);
            }
        }
    }

    TinyValVectorFull(Me&& moved) : Base(moved) {
        moved.setNull();
    }

    Me& operator=(T singleValue) {
        clear();
        Base::setValue(singleValue);
        return *this;
    }

    Me& operator=(const Me& copy) {
        clear();
        if (copy.Base::isPointer()) {
            auto* vector = copy.Base::getPointer();
            if (vector) {
                auto* copiedVector = newVector(*vector);
                Base::setPointer(copiedVector);
                return *this;
            }
        }
        Base::operator=(copy);
        return *this;
    }

    static Me Clone(const Me& copy) {
        if (copy.template is<VectorT*>()) {
            auto* vector = copy.template get<VectorT*>();
            if (vector) {
                Me me;
                auto* copiedVector = me.newVector(*vector);
                me.setVector(copiedVector);
                return me;
            }
        } else {
            return copy; // no vector, dont need to worry about actually copying anything
        }
    }

    static Me Move(Me&& moved) {
        Me me = moved;
        moved.setNull();
        return me;
    }
private:

    void setNull() {
        Base::setPointer((VectorT*)nullptr);
    }

    VectorT* newVector(const VectorT& value = {}) {
        return new VectorT(value);
    }

    void setVector(VectorT* vector) {
        Base::setPointer(vector);
    }

    // assert failure if type is single value
    VectorT* getOrMakeVector() {
        auto* vector = Base::getPointer();
        if (!vector) {
            // was completely empty before
            vector = newVector();
            Base::setPointer(vector);
        }
        return vector;
    }
public:
    /* Special methods */

    // returns true if the vector contains a single element
    bool single() const {
        return this->size() == 1;
    }

    T& getSingleValue() {
        return Base::getValue();
    }

    const T& getSingleValue() const {
        return const_cast<Me*>(this)->getSingleValue();
    }

    VectorT& getVector() {
        return *Base::getPointer();
    }

    const VectorT& getVector() const {
        return const_cast<Me*>(this)->getVector();
    }

    /* Standard vector functions */

    T* data() {
        if (Base::isValue()) {
            return &getSingleValue();
        } else {
            auto* vector = Base::getPointer();
            return vector ? vector->data() : nullptr;
        }
    }

    const T* data() const {
        return const_cast<Me*>(this)->data();
    }

    T& operator[](int index) {
        assert(index < this->size() && "Tiny val vector index out of bounds!");
        return data()[index];
    }

    const T& operator[](int index) const {
        assert(index < this->size() && "Tiny val vector index out of bounds!");
        return data()[index];
    }

    bool empty() const {
        return this->size() == 0;
    }

    void push_back(T value) {
        if (Base::isPointer()) {
            auto* vector = Base::getPointer();
            if (!vector)
                Base::setValue(value);
            else
                vector->push_back(value);
        } else {
            // one element already around
            // move to vector
            // allocate a vector and put existing value in
            T existingValue = getSingleValue();
            VectorT* vector = newVector({existingValue, value});
            // replace single value with vector
            Base::setPointer(vector);
        }
    }

    void append(const T* values, int count) {
        assert(count >= 0 && "negative element count!");
        if (count == 0) return;
        if (count == 1) {
            push_back(values[0]);
        } else {
            // need vector
            bool vectorType = Base::isPointer();
            VectorT* vector;
            if (vectorType) {
                vector = getOrMakeVector();
            } else {
                T existingValue = getSingleValue();
                vector = newVector({existingValue});
            }
            vector->append(values, values + count);
        }
    }

    void append(ArrayRef<T> array) {
        append(array.data(), array.size());
    }

    void append(int numCopies, T value) {
        assert(numCopies >= 0 && "negative element count!");
        if (numCopies == 0) return;
        if (numCopies == 1) {
            push_back(value);
        } else {
            // need vector
            bool vectorType = Base::isPointer();
            VectorT* vector;
            if (vectorType) {
                vector = getOrMakeVector();
            } else {
                T existingValue = getSingleValue();
                vector = newVector({existingValue});
            }
            vector->append(numCopies, value);
        }
    }

    void pop_back() {
        if (Base::isValue()) {
            // popped only element
            Base::setPointer(nullptr);
        } else {
            VectorT* vector = Base::getPointer();
            assert(vector != nullptr && "Popped back empty vector!");
            vector->pop_back();
        }
    }

    int size() const {
        if (Base::isValue()) return 1;

        VectorT* vector = Base::getPointer();
        return vector ? vector->size() : 0;
    }

    int capacity() const {
        if (Base::isValue()) return 1;
        
        VectorT* vector = Base::getPointer();
        // always has 1 capacity built in
        return vector ? std::max(vector->capacity(), 1) : 1;
    }

    void resize(int newSize, T value = {}) {
        if (newSize <= 0) {
            setNull();
            return;
        } else if (Base::isValue()) {
            // is just one value currently
            if (newSize > 1) {
                // move to vector
                auto* vector = newVector();
                vector->resize(newSize, value);
                T existingValue = getSingleValue();
                (*vector)[0] = existingValue;
                Base::setPointer(vector);
            }
        } else {
            auto* vector = getOrMakeVector();
            vector->resize(newSize, value);
        }
    }

    void reserve(int newCapacity) {
        if (newCapacity <= 1) return; // capacity is always atleast 1 because of the built in storage
        if (Base::isPointer()) {
            auto* vector = getOrMakeVector();
            vector->reserve(newCapacity);
        } else if (newCapacity > 1) {
            // is just one value currently
            // move to vector
            T existingValue = getSingleValue();
            auto* vector = newVector();
            vector->reserve(newCapacity);
            vector->push_back(existingValue);
            Base::setPointer(vector);
        }
    }

    void clear() {
        if (Base::isPointer()) {
            auto* vector = Base::getPointer();
            if (vector) delete vector;
        }
        setNull();
    }

    operator ArrayRef<T>() const {
        if (Base::isValue()) {
            return ArrayRef(&getSingleValue(), 1);
        } else {
            auto* vector = Base::getPointer();
            return vector ? ArrayRef(vector->data(), vector->size()) : ArrayRef<T>{};
        }
    }

    operator MutArrayRef<T>() {
        if (Base::isValue()) {
            return MutArrayRef(&getSingleValue(), 1);
        } else {
            auto* vector = Base::getPointer();
            return vector ? MutArrayRef(vector->data(), vector->size()) : MutArrayRef<T>{};
        }
    }

    ~TinyValVectorFull() {
        clear();
    }
};

template<typename T, uint8_t UnusedBitIndex = sizeof(T) * 8, bool UnusedBitState = false, bool AssertBitUnused = false>
using TinyValVector = TinyValVectorFull<T, UnusedBitIndex, UnusedBitState, AssertBitUnused>;

// like vector but can't grow or anything
class TinyValArray {

};

#endif