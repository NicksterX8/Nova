#ifndef GUI_COMPONENTS_INCLUDED
#define GUI_COMPONENTS_INCLUDED

#include "rendering/textures.hpp"
#include "utils/vectors.hpp"
#include "ecs-gui.hpp"
#include "rendering/text.hpp"
#include "ECS/generic/components.hpp"

namespace GUI {

namespace EC {

using ComponentID = GECS::ComponentID;

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
    #define GUI_REGULAR_COMPONENTS_LIST Texture, Background, Border, ViewBox, Button, Text, Child
    #define GUI_PROTO_COMPONENTS_LIST PlaceholderPr
    #define GUI_COMPONENTS_LIST GUI_REGULAR_COMPONENTS_LIST, GUI_PROTO_COMPONENTS_LIST
    GEN_IDS(ComponentIDs, ComponentID, GUI_COMPONENTS_LIST, Count);
}


#define BEGIN_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = false;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define BEGIN_PROTO_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = true;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_PROTO_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");


BEGIN_COMPONENT(Button)
    void(*onClick)(void);
    bool clickedThisFrame;
    bool pressed; // if the button is down

    Button(void(*onClick)(void)) : onClick(onClick), clickedThisFrame(false), pressed(false) {

    }
END_COMPONENT(Button)

BEGIN_COMPONENT(ViewBox)
    Box box;
END_COMPONENT(ViewBox)

BEGIN_COMPONENT(Background)
    SDL_Color color;
END_COMPONENT(Background)

BEGIN_COMPONENT(Border)
    SDL_Color color;
    float thickness; // pixels
END_COMPONENT(Border);

BEGIN_COMPONENT(Texture)
    TextureID texture;
    Box texBox;
END_COMPONENT(Texture)

BEGIN_COMPONENT(Child)
    ElementID parent;
END_COMPONENT(Child)

BEGIN_COMPONENT(Text)
    const char* text;
    TextFormattingSettings formatting;
    TextRenderingSettings renderSettings;
END_COMPONENT(Text)

BEGIN_PROTO_COMPONENT(PlaceholderPr)
    int hi;
END_PROTO_COMPONENT(PlaceholderPr)

}

}

#endif