#ifndef PROTOTYPE_INCLUDED
#define PROTOTYPE_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include "My/Vec.hpp"
#include "My/HashMap.hpp"
#include "utils/nonnull.hpp"
#include "components/components.hpp"

struct EntityWorld;

#include "SEntity.hpp"

using Uint32 = uint32_t;

struct EntityPrototypeBase {

    virtual void Created() const {}

    virtual void Serialize() const {}

    virtual void Destroyed() const {}
};

struct Prototype {
    Uint32 id;

    const char* getIdentifier() {
        return "john";
    }

    static Uint32 getIdFromIdentifer(const char* identifer) {
        return 2;
    }
};

template<class T>
struct StaticVal {
    T val;
    Uint32 id;

    StaticVal(const T& val, Uint32 id) {
        this->val = val;
        this->id = id;
    }

    const char* name() {
        return /* val->getName(); */ NULL;
    }

    virtual void Serialize() {
        // serialize getIdentifer()
        T::serialize(val);
    }

    virtual void Deserialize() {
        // 
    }
};

int health = 3;

template<class T>
struct StaticRef {
    const T* val;
    Uint32 id;
};

namespace PrototypeIDs {
    enum IDs {
        Explosive,
        Zombie
    };
};

int myModel = 2;

using ObjectID = Uint32; // 64 bits is unnecessary

struct PointerTag {
    std::string name; // optional
    ObjectID id;
    bool usingNameOrId;

    PointerTag(ObjectID ID) : id(ID), usingNameOrId(false) {}

    PointerTag(const std::string&) : name(name), usingNameOrId(true) {}
};
/*
template<typename T>
struct TaggedPointer {
    T* ptr;

    TaggedPointer(T* _ptr) {

    } 
};
*/
using TaggedPointer = void*;

template<typename T>
struct ResourceRef {
    TaggedPointer pointer;

    ResourceRef() : pointer(nullptr) {}

    ResourceRef(TaggedPointer pointer) : pointer(pointer) {

    }

    T& operator*() {
        return *static_cast<T*>(pointer);
    }
    const T& operator*() const {
        return *static_cast<T*>(pointer);
    }

    operator T*() {
        return pointer;
    }

    operator const T*() const {
        return pointer;
    }
};

struct PointerTagManager {
    My::Vec<void*> pointers; // maybe not necessary
    My::Vec<ObjectID> ids; // maybe not necessary
    My::Vec<const char*> names;

    My::Vec<Uint32> openIDs; // fill this with numbers

    My::HashMap<void*, ObjectID> registry;
    //ObjectID idCounter = 0;

    PointerTag getTagFromPtr(TaggedPointer ptr) {
        return getTag(ptr);
    }

    TaggedPointer getPtrFromTag(PointerTag tag) {
        return pointers[tag.id];
    }

    template<typename T>
    ResourceRef<T> ref(T* value, const char* name = NULL) {
        add(value, name);
        return value;
    }

    void add(void* ptr, Nonnull<const char*> name) {
        name = nullptr;
        registry.update(ptr, openIDs.back());
        pointers.push(ptr);
        names.push(name);
        assert(!openIDs.empty());
        
        openIDs.pop();
    }

    PointerTag getTag(TaggedPointer taggedPtr) {
        ObjectID id = *registry.lookup((void*)taggedPtr);
        return PointerTag(id);
    }

    void* get(const char* name) {
        // do the thing
        for (int i = 0; i < names.size; i++) {
            if (strcmp(name, names[i]) == 0) {
                return pointers[i];
            }
        }
    }
};

inline int fkdajslfkja() {
    PointerTagManager manager;
    int* globalInt = new int(12);
    manager.add(globalInt, "globalInt");

    // later


}

int* global = new int(2);

PointerTagManager tagger;

struct ComponentWithPtr {
    ResourceRef<int> model;

    ComponentWithPtr() {}

    void init() {
        model = tagger.ref(global);
    }

    void serialize(/* Outputter out */) {
        PointerTag tag = tagger.getTagFromPtr(this->model);

        // out(tag)
    }

    static void deserialize(Inputter& in, ComponentWithPtr* component /* Inputter in */) {
        // Uint32 id; in(&id);
        // or
        // char name[100]; in(name);
        // or
        PointerTag tag = IO::pointerTag(in);
 
        // then

        // tagger should already have pointer tagged by the time this is run
        component->model = ResourceRef<int>(tagger.getPtrFromTag(tag));
    }
};

struct Outputter {
    // some state ...

    size_t out(size_t bytes, const void* data) {
        return 0;
    }
};

struct Inputter {
    size_t in(size_t bytes, void* data) {
        return 0;
    }

    void* in(size_t bytes) {
        return nullptr;
    }

    template<typename T>
    T& in() {
        return *nullptr;
    }
};

namespace InOut {

template<class T>
size_t out(const T& val, Outputter& output) {
    return output.out(sizeof(T), &val);
}

template<class T>
size_t out(size_t count, const T* data, Outputter& output) {
    return output.out(count * sizeof(T), data);
}

#define OUT_VAL(type, val) InOut::out<type>(val, out)
#define OUT_PTR(type, ptr, count) InOut::out<type>(count, ptr, out)

template<class T>
inline T in(Inputter& input) {
    T val;
    size_t bytesWritten = input.in(sizeof(T), &val);
    if (bytesWritten != sizeof(T)) {
        // err
    }
    return val;
}

template<class T>
inline size_t in(size_t count, T* data, Inputter& input) {
    return input.in(count * sizeof(T), data);
}

template<class T>
inline T* in(size_t count, Inputter& input) {
    return (T*)input.in(count);
}

}

#define IN_VAL(type, var) type var = InOut::in<type>(in)
#define IN_READ(type, ptr, count) InOut::in<type>(count, ptr, in)
#define IN_ALLOC(type, var, count) type* var = InOut::in<type>(count, in)
#define IN_FREE(var) free(var)

namespace IO {


    inline void string(Outputter& out, const std::string& str) {
        OUT_VAL(size_t, str.size());
        OUT_PTR(char, str.data(), str.size());
    }

    inline std::string string(Inputter& in) {
        IN_VAL(size_t, size);

        IN_ALLOC(char, data, size);
        auto str = std::string(data, size);

        IN_FREE(data);
        return str;
    }

    inline void string(Inputter& in, std::string& str) {
        str = string(in);
    }

    inline PointerTag pointerTag(Inputter& in) {
        bool kkk = in.in<bool>();
        if (kkk) {
            ObjectID id = in.in<ObjectID>();
            return PointerTag(id);
        } else {
            auto _string = string(in);
            return PointerTag(_string);
        }
    }

    inline void pointerTag(Outputter& out, PointerTag tag) {
        
    }
}

struct EntityNamePrototype : EntityPrototypeBase {
    std::string name;
    std::string displayName;

    void Created(EntityWorld* ecs, Entity entity) const {

    }

    void Serialize(EntityWorld* ecs, Entity entity, Outputter& out) const {
        IO::string(out, name);
        IO::string(out, displayName);
    }

    void Deserialize(EntityWorld* ecs, Entity entity, Inputter& in) {
        IO::string(in, name);
        IO::string(in, displayName);
    }
};

struct ExplosivePrototype : EntityPrototypeBase {
    EC::Explosion explosion;

    void Serialize(Outputter& out) const {
        OUT_VAL(EC::Explosion, explosion);
    }

    void DeserializeEntity(EntityWorld* ecs, Entity entity, Inputter& in) {
        /*
        EC::Explosive* explosive = ecs->Get<EC::Explosive>(entity);
        if (explosive) {
            //*explosive->explosion = explosion;
        }*/
    }
};

template<class T>
struct GenericProtoComponent : EC::EntityComponent<GenericProtoComponent<T>> {
    T* ptr;

    void Serialize() {}
};

struct ExplosiveProtoComponent : EC::EntityComponent<ExplosiveProtoComponent> {
    ExplosivePrototype* ptr;

    void Serialize(Outputter& out) const {

    }
};

struct NamePrototypeComponent : EC::EntityComponent<NamePrototypeComponent> {
    EntityNamePrototype* prototype;

    static int Serialize(const NamePrototypeComponent* components, Uint32 count, EC::SerializerOutput output) {
        return 0;
    }

    static int Deserialize(NamePrototypeComponent* components, Uint32 count, const char* serialized) {
        return 0;
    }
};



class TreePrototype : EntityPrototypeBase {

};

#endif