

#include "MyInternals.hpp"
#include "Array.hpp"
#include "../utils/common-macros.hpp"

MY_CLASS_START
/*
template<typename ReturnType, typename Capture, typename ...Params> // captures
struct Lambda {
    Capture capture;
    using Function = ReturnType(Capture&, Params...);
    Function* function;

    ReturnType call(Params ...params) {
        return function(capture, params...);
    }

    Lambda(Function* func, const Capture& capture) : function(func), capture(capture) {
        
    }
};

template<typename ReturnType, typename... Params>
struct MyFunction {
    using Self = MyFunction<ReturnType, Params...>;
    using Function = ReturnType(void*, Params...);
    Function* function;
    void* capture;

    template<typename... Captures>
    static Self create(Function* function, Captures ...captures) {
        Self self;
        self.function = function;
        std::tuple<Captures...>* tuple = new std::tuple<Captures...>;
        //tuple = std::make_tuple<Captures...>(captures...);
        self.capture = tuple;
    }
};

inline void testLambda() {
    void* ptr;
    Lambda<int, void*> lambda([](void*& cap) -> int {

    }, ptr);
//
    char c = 'a';
    auto myFunc = MyFunction<bool, int, float>::
    create([](void* captures, int i, float f) -> bool {
        
    }, c);
}
*/

MY_CLASS_END