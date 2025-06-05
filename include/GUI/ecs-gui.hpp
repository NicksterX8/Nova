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

inline void traverseTree(GuiTreeNode* node, const std::function<bool(GuiTreeNode* child, GuiTreeNode* parent)>& nodeOperation) {
    for (int c = 0; c < node->childCount; c++) {
        nodeOperation(&node->children[c], node);
        traverseTree(&node->children[c], nodeOperation);
    }
}

inline void addChildNode(GuiTreeNode* node, GuiTreeNode* child) {
    const int newSize = node->childCount + 1;
    if (newSize > node->capacity) {
        int newCapacity = (node->capacity * 2 > newSize) ? node->capacity * 2 : newSize;
        GuiTreeNode* newChildren = Realloc<GuiTreeNode>(node->children, newCapacity);
        node->children = newChildren;
        node->capacity = newCapacity;
    }
    memcpy(&node->children[node->childCount++], child, sizeof(GuiTreeNode));
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

    // void changeParents(Element parent, Element child) {
    //     if (parent == child) {
    //         LogError("Element cannot adopt itself!");
    //         return;
    //     }

    //     GuiTreeNode* oldParent = nullptr;
    //     GuiTreeNode* newParent = nullptr;
    //     GuiTreeNode* childNode = nullptr;
    //     traverseTree(&root, [&](GuiTreeNode* node, GuiTreeNode* parentNode){
    //         if (node->e == child) {
    //             oldParent = parentNode;
    //             childNode = node;
    //         }
    //         else if (node->e == parent) {
    //             newParent = node;

    //             GuiTreeNode childNode(child);
                
    //         }

    //         if (oldParent && newParent) {
    //             addChildNode(newParent, childNode);
    //             oldParent->children.erase

    //             return true;
    //         }

    //         return false;
    //     });
    // }

    void adopt(Element parent, Element child) {
        if (parent == child) {
            LogError("Element cannot adopt itself!");
            return;
        }

        traverseTree(&root, [&](GuiTreeNode* node){
            if (node->e == parent) {
                GuiTreeNode childNode(child);
                addChildNode(node, &childNode);
                return true;
            }
            return false;
        });
    }

    Element newElement(GECS::PrototypeID prototype, Element parent = NullElement) {
        Element element = GECS::ElementManager::newElement(prototype);
        if (parent != NullElement) {
            adopt(parent, element);
        }
        return element;
    }

    bool addName(Element element, const char* name) {
        if (!name) {
            LogError("No name given to add");
            return false;
        }
        if (strlen(name) > GUI_ELEMENT_MAX_NAME_SIZE) {
            LogWarn("Element name too long! Name will be truncated. Name: \"%s\"", name);
        }
        EC::Name nameEc;
        strncpy(nameEc.name, name, GUI_ELEMENT_MAX_NAME_SIZE);
        return addComponent<EC::Name>(element, nameEc);
    }

    void destroy() {
        destroyTree(&root);

        GECS::ElementManager::destroy();
    }

    Element getNamedElement(const char* name) const {
        Element namedElement = NullElement;
        forEachElement<EC::Name>([&](Element e){
            auto* nameEc = getComponent<EC::Name>(e);
            if (My::streq(nameEc->name, name)) {
                namedElement = e;
            }
        });
        return namedElement;
    }

    Element getNamedElement(std::string name) const {
        return getNamedElement(name.c_str());
    }

};


}

#endif