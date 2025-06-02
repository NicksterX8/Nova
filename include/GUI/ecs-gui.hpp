#ifndef GUI_ECS_GUI_INCLUDED
#define GUI_ECS_GUI_INCLUDED

#include "ECS/generic/ecs.hpp"

namespace GUI {

using ComponentID = GECS::ComponentID;


using Element = GECS::Element;
using Prototype = GECS::Prototype;

using ElementID = GECS::ElementID;

struct GuiManager : GECS::ElementManager {

    GuiManager() {}

    GuiManager(GECS::ComponentInfoRef componentInfo, int numPrototypes)
     : GECS::ElementManager(componentInfo, numPrototypes) {

     }

    void destroy() {
        GECS::ElementManager::destroy();
    }

};


}

#endif