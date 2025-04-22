#ifndef GUI_COMPONENTS_INCLUDED
#define GUI_COMPONENTS_INCLUDED

#include "rendering/textures.hpp"
#include "utils/vectors.hpp"
#include "ecs-gui.hpp"

namespace GUIEC {

using ComponentID = GuiEcs::ComponentID;

/*
Types of elements:
Button
Text Box
Hot bar
Free text (like fps counter)
Scale selector (like for volume)
Checkbox

*/

/*
components:
render
button
text

*/

namespace ComponentIDs {
    #define GUI_REGULAR_COMPONENTS_LIST Display, Button, Text, Child
    #define GUI_PROTO_COMPONENTS_LIST PlaceholderPr
    #define GUI_COMPONENTS_LIST GUI_REGULAR_COMPONENTS_LIST, GUI_PROTO_COMPONENTS_LIST
    GEN_IDS(ComponentIDs, ComponentID, GUI_COMPONENTS_LIST, Count);
}

BEGIN_COMPONENT(Button)
    bool clickedThisFrame;
    bool pressed; // if the button is down
    void(*onClick(void));
END_COMPONENT(Button)

BEGIN_COMPONENT(Display)
    TextureID texture;
    Box viewbox;
END_COMPONENT(Display)

BEGIN_COMPONENT(Child)
    GuiEcs::ElementID parent;
END_COMPONENT(Child)

BEGIN_COMPONENT(Text)
    int stringID;
END_COMPONENT(Text)

BEGIN_PROTO_COMPONENT(PlaceholderPr)
    int hi;
END_PROTO_COMPONENT(PlaceholderPr)

}

#endif