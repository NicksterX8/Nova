#ifndef GUI_ECS_GUI_INCLUDED
#define GUI_ECS_GUI_INCLUDED

#include "ECS/generic/ecs.hpp"
#include "components.hpp"
#include "prototypes.hpp"

namespace GUI {

using ComponentID = GECS::ComponentID;


using Element = GECS::Element;
constexpr Element NullElement = GECS::NullElement;

using ElementID = GECS::ElementID;

struct GuiTreeNode {
    Element e;
    GuiTreeNode* children;
    int childCount;
    int capacity;

    GuiTreeNode() {}

    GuiTreeNode(Element e) : e(e), children(nullptr), childCount(0), capacity(0) {}
};

inline void destroyTree(GuiTreeNode* node) {
    for (int c = 0; c < node->childCount; c++) {
        destroyTree(&node->children[c]);
    }
    Free(node->children);
}

inline void traverseTree(GuiTreeNode* node, const std::function<bool(GuiTreeNode*)>& nodeOperation) {
    if (nodeOperation(node)) {
        return;
    }
    for (int c = 0; c < node->childCount; c++) {
        traverseTree(&node->children[c], nodeOperation);
    }
}

struct GuiManager : GECS::ElementManager {

    Element screen = NullElement;

    GuiTreeNode root = NullElement;

    Element hoveredElement = NullElement;

    GuiManager() {}

    GuiManager(GECS::ComponentInfoRef componentInfo, int numPrototypes)
     : GECS::ElementManager(componentInfo, numPrototypes) {
        screen = GECS::ElementManager::newElement(ElementTypes::Normal);

        addComponent(screen, EC::ViewBox{Box{Vec2(0), Vec2(INFINITY)}});
        addComponent(screen, EC::Hidden{});

        root.e = screen;
    }

    void adopt(Element parent, Element child) {
        traverseTree(&root, [&](GuiTreeNode* node){
            if (node->e == parent) {
                GuiTreeNode childNode(child);
                const int newSize = node->childCount + 1;
                if (newSize > node->capacity) {
                    int newCapacity = (node->capacity * 2 > newSize) ? node->capacity * 2 : newSize;
                    GuiTreeNode* newChildren = Realloc<GuiTreeNode>(node->children, newCapacity);
                    node->children = newChildren;
                    node->capacity = newCapacity;
                }
                
                memcpy(&node->children[node->childCount++], &childNode, sizeof(GuiTreeNode));
                return true;
            }
            return false;
        });
    }

    Element newElement(GECS::PrototypeID prototype, Element parent) {
        Element element = GECS::ElementManager::newElement(prototype);
        if (parent == NullElement) {
            parent = root.e;
        }
        adopt(parent, element);
        return element;
    }

    void destroy() {
        destroyTree(&root);

        GECS::ElementManager::destroy();
    }

    Element getNamedElement(std::string name) const {
        Element namedElement = NullElement;
        forEachElement<EC::Name>([&](Element e){
            auto* nameEc = getComponent<EC::Name>(e);
            if (My::streq(nameEc->name, name.c_str())) {
                namedElement = e;
            }
        });
        return namedElement;
    }

};


}

#endif