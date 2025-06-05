#ifndef GUI_COMPONENTS_INCLUDED
#define GUI_COMPONENTS_INCLUDED

#include "rendering/textures.hpp"
#include "utils/vectors.hpp"
#include "rendering/text.hpp"
#include "ECS/generic/components.hpp"
#include "actions.hpp"

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
    #define GUI_REGULAR_COMPONENTS_LIST Name, SimpleTexture, Numbered, Background, \
        Border, ViewBox, Hidden, MaxSize, MinSize, Button, Text, ChildOf, Hover, \
        SizeConstraint, AlignmentConstraint, StackConstraint
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
    GuiAction onClick = nullptr;
    bool pressed = false; // if the button is down
END_COMPONENT(Button)

#define GUI_ELEMENT_MAX_NAME_SIZE 64

BEGIN_COMPONENT(Name)
    char name[GUI_ELEMENT_MAX_NAME_SIZE];
END_COMPONENT(Name)

BEGIN_COMPONENT(MaxSize)
    IVec2 maxSize;
END_COMPONENT(MaxSize)

BEGIN_COMPONENT(MinSize)
    IVec2 minSize;
END_COMPONENT(MinSize)

BEGIN_COMPONENT(Hover)
    bool changeColor = false;
    SDL_Color newColor = {0,0,0,0};
    GuiAction onHover = nullptr; // when an element is first hovered on (first frame its on it)
END_COMPONENT(Hover)

BEGIN_COMPONENT(ViewBox)
    Box box = {{0,0},{0,0}}; // relative box
    Box absolute = {{0,0},{0,0}}; // where element was actually displayed
    int level = 0;
    bool hide = false;
END_COMPONENT(ViewBox)

BEGIN_COMPONENT(Hidden)
END_COMPONENT(Hidden)

BEGIN_COMPONENT(Background)
    SDL_Color color;
END_COMPONENT(Background)

BEGIN_COMPONENT(Border)
    SDL_Color color = {0,0,0,0};
    Vec2 strokeIn = {0,0};
    Vec2 strokeOut = {0,0};
END_COMPONENT(Border);

BEGIN_COMPONENT(SimpleTexture)
    TextureID texture = TextureIDs::Null;
    Box texBox = {{0,0},{0,0}};
END_COMPONENT(SimpleTexture)

BEGIN_COMPONENT(ChildOf)
    GECS::Element parent = GECS::NullElement;
END_COMPONENT(ChildOf)

BEGIN_COMPONENT(Text)
    const char* text = nullptr;
    TextFormattingSettings formatSettings;
    TextRenderingSettings renderSettings;
END_COMPONENT(Text)

BEGIN_COMPONENT(Numbered)
    int number = -1;
END_COMPONENT(Numbered)

BEGIN_COMPONENT(SizeConstraint)
    Vec2 maxSize = Vec2(INFINITY);
    Vec2 minSize = Vec2(0.0f);
    Vec2 relativeSize = Vec2(INFINITY); // 0 to 1
END_COMPONENT(SizeConstraint)

BEGIN_COMPONENT(AlignmentConstraint)
    TextAlignment alignment;
END_COMPONENT(AlignmentConstraint)

BEGIN_COMPONENT(StackConstraint)
    bool horizontal = false;
    bool vertical = false;
END_COMPONENT(StackConstraint)


BEGIN_PROTO_COMPONENT(PlaceholderPr)
    int hi;
END_PROTO_COMPONENT(PlaceholderPr)

}

}

#endif