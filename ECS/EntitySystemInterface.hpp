#ifndef ENTITY_SYSTEM_INTERFACE_INCLUDED
#define ENTITY_SYSTEM_INTERFACE_INCLUDED

#include "EntityManager.hpp"
#include "Entity.hpp"

extern unsigned int SystemCount;

template<class T>
Uint32 getSystemID() {
    static Uint32 id = SystemCount++;
    return id;
}

class EntitySystemInterface {
public:
    _Entity *entities = NULL;
    Uint32 nEntities = 0;
    Uint32 reservedEntities = 0;
    
    bool mustExecuteAfterStructuralChanges = false;
    bool containsStructuralChanges = false;

    EntitySystemInterface();

    EntitySystemInterface(Uint32 reserve);

    virtual bool Query(ComponentFlags entitySignature) const = 0;

    virtual void Update() = 0;

    void AddEntity(_Entity entity);

    int RemoveEntityLocally(Uint32 localIndex);

    int RemoveEntity(_Entity entity);

    void EntitiesWereRemoved(EntityManager* em);

    void RemoveAllEntities();

    Uint32 NumEntities() {
        return nEntities;
    }

private:
    void resize(Uint32 reserve);
};

#endif