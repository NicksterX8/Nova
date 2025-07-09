

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

MY_CLASS_END