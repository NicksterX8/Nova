

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
/*
struct Big {
    char a;
    int b;
    int render;
    bool align;

    Big& setA(char newA) {
        this->a = newA;
        return *this;
    }
    Big& setB(int newB) {
        this->b = newB;
        return *this;
    }

    enum class Setting {
        Align,
        Scale,
        Color
    };

    template<typename T>
    struct ParamValue {
        Setting setting;
        T value;
    };

    static ParamValue<glm::vec4> color(glm::vec4 c) {
        return {Setting::SDL_Color, c};
    }
    static ParamValue<int> align(int a) {
        return {Setting::Align, a};
    }

    template<Setting setting, typename T>
    static ParamValue<T> makeSetting(T val) {
        return {setting, val};
    }

    static auto scale(float s) { return makeSetting<Setting::Scale, float>(s); }

    template<typename ...Ts>
    void set(Ts ...params) {
        
    }
};

inline int testestts() {
    Big big;
    big.set(Big::scale(2), Big::color({1,0,0,1}));
}
*/

MY_CLASS_END