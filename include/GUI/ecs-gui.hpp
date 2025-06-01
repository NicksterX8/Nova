#ifndef GUI_ECS_GUI_INCLUDED
#define GUI_ECS_GUI_INCLUDED

#include "ECS/generic/ecs.hpp"

namespace GUI {

using ComponentID = GECS::ComponentID;


using Element = PECS::Entity;
using Prototype = PECS::Prototype;

using PECS::getComponent;
using PECS::getPrototype;
using PECS::addComponent;

namespace PrototypeIDs {
    #define GUI_ELEMENT_PROTOTYPE_LIST Normal
    GEN_IDS(ItemTypes, Uint16, GUI_ELEMENT_PROTOTYPE_LIST, Count);  // 65k types should be way more than enough
}

using ElementID = Uint32;

struct GuiManager : PECS::EntityManager {
    My::HashMap<ElementID, Element> elements;

    GuiManager() {}

    GuiManager(GECS::ComponentInfoRef componentInfo, int numPrototypes)
     : PECS::EntityManager(componentInfo, numPrototypes) {

     }


};

inline Element makeElement(GuiManager& gui) {
    Element element = PECS::makeEntity(PrototypeIDs::Normal, gui);
    gui.elements.insert()
    return element;
}


}

#endif