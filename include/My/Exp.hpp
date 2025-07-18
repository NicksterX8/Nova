

#include "MyInternals.hpp"
#include "Array.hpp"
#include "../utils/common-macros.hpp"

MY_CLASS_START

template<typename Return, typename... Args>
struct Function {
    char captureStorage[24];
    using FuncPtr = Return(*)(Args...);
    FuncPtr funcPtr;

    template<typename Callable>
    Function() {
        
    }
};

template<typename Function, typename >
Function makeFunction()


struct ExperimentalAny {
    void* buffer;

    template<typename T>
    ExperimentalAny(const T& value) {
        buffer = new T(value);
    }

    template<typename T>
    const T& read() const {
        return *(T*)buffer;
    }

    template<typename T>
    T& read() {
        return *(T*)buffer;
    }
};

MY_CLASS_END