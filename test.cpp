#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
 
using namespace std;

struct S {
    int y;
};

#include <SDL2/SDL.h>

typedef Uint16 ItemID;

namespace Items {
    namespace Flags {
        enum Flags {
            IsWettable=1,
            IsSpoilable=2
        };
    }
}

struct ItemFlagNameIdPair {
    const char* name;
    Uint32 id;
};

namespace ItemFlags {
    enum Flags {
        Wettable
    };
}

ItemFlagNameIdPair itemFlags[] = {
    {"wettable", ItemFlags::Wettable}
};

struct ItemType {};

namespace ItemComponents {
    Uint32 itemComponentIdCounter = 0;

    template<class T>
    Uint32 itemComponentID() {
        static Uint32 id = itemComponentIdCounter++;
        return id;
    }

    struct Wettable {
        float wetness;
    };

    struct Spoilable {
        float spoilage;
    };

    struct Breakable {
        float durability;
    };
};

struct ItemTypeDataStruct {
    const char* name;
    SDL_Texture* texture;
    Uint32 flags;
    
};

template<class Type, class Component>
void* componentGetter(ItemType* type) {
    return static_cast<Component*>(static_cast<Type*>(type));
}

struct GranolaBarDataType : ItemTypeDataStruct {
    GranolaBarDataType() {
        name = "granola-bar";
        texture = NULL;

        
    }
};

#define NUM_ITEM_TYPES 1

ItemTypeDataStruct ItemTypeData[NUM_ITEM_TYPES];

struct Item {
    ItemID id;
    ItemType* type;

    template<class T>
    T* getComponent() {
        if (ItemTypeData[id].has<T>()) {
            return 
        }
        return NULL;
    }

};

struct ItemStack {
    Uint32 quantity;
    Item item;
};

namespace {

using namespace ItemComponents;

struct GranolaBar : ItemType, Wettable, Spoilable {

    GranolaBar() {
        spoilage = 0.0f;
        wetness = 100;
    }
};

}

struct A {
    int a = 3;
};

struct B {
    int b = 2;
};

struct AB : public A, public B {
    int ab = 6;
};

int main() {
    ItemStack stack;
    cout << "size of stack: " << sizeof(stack) << "\n";
    cout << "size of id: " << sizeof(ItemID) << "\n";
    cout << "size of item: " << sizeof(Item) << "\n";

    AB ab;
    A* a = static_cast<A*>(&ab);
    B* b = static_cast<B*>(&ab);
    cout << "ab.a: " << ab.a << " a.a: " << a->a << " b.b: " << b->b << "\n";

    GranolaBar bar;
    bar.spoilage = 10.0f;

    return 0;
}