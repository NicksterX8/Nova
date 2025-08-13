#ifndef GUI_ECS_GUI_INCLUDED
#define GUI_ECS_GUI_INCLUDED

#include "ECS/EntityManager.hpp"
#include "components.hpp"
#include "prototypes.hpp"
#include "ECS/System.hpp"

namespace GUI {

using ComponentID = ECS::ComponentID;

using Element = ECS::Entity;
constexpr Element NullElement = ECS::NullEntity;

using ElementID = ECS::EntityID;

struct GuiTreeNode {
    Element e = NullElement;
    ElementID parent = NullElement.id;
    ElementID* children = nullptr;
    int childCount = 0;
    int capacity = 0;

    GuiTreeNode() {}

    GuiTreeNode(Element e, ElementID parent) : e(e), parent(parent) {}
};

inline void addChildNode(GuiTreeNode* node, ElementID child) {
    const int newSize = node->childCount + 1;
    if (newSize > node->capacity) {
        int newCapacity = (node->capacity * 2 > newSize) ? node->capacity * 2 : newSize;
        ElementID* newChildren = Realloc<ElementID>(node->children, newCapacity);
        node->children = newChildren;
        node->capacity = newCapacity;
    }
    std::copy(&child, &child + 1, &node->children[node->childCount++]);
}

struct GuiManager : ECS::EntityManager {
    Element screen;
    My::HashMap<ElementID, GuiTreeNode> treeMap;
    Element hoveredElement;
    ECS::Systems::SystemManager systemManager;

    GuiTreeNode* getTreeNode(ElementID elementID) const {
        return treeMap.lookup(elementID);
    }

    void destroyTree(GuiTreeNode* node) {
        for (int c = 0; c < node->childCount; c++) {
            destroyTreeElement(node->children[c]);
        }
        Free(node->children);
    }

    inline void destroyTreeElement(ElementID element) {
        GuiTreeNode* node = treeMap.lookup(element);
        for (int c = 0; c < node->childCount; c++) {
            destroyTreeElement(node->children[c]);
        }
        Free(node->children);
    }

    void init(ArrayRef<ECS::ComponentInfo> componentInfo, int numPrototypes) {
        screen = NullElement;
        hoveredElement = NullElement;
        treeMap = decltype(treeMap)(32);
        ECS::EntityManager::init(componentInfo, numPrototypes);
        screen = ECS::EntityManager::createEntity(ElementTypes::Normal);
        treeMap.insert(screen.id, GuiTreeNode(screen, NullElement.id));

        addComponent(screen, EC::ViewBox{Box{Vec2(0), Vec2(INFINITY)}, .visible = false});
        addComponent(screen, EC::DisplayBox{});
        addComponent(screen, EC::Hidden{});

        setComponentDestructor(EC::Update::ID, [this](Entity entity){
            auto* component = getComponent_<EC::Update>(entity);
            component->destroy();
        });

        setComponentDestructor(EC::Button::ID, [this](Entity entity){
            auto* component = getComponent_<EC::Button>(entity);
            component->destroy();
        });
    }

    void removeFromTree(Element element) {
        GuiTreeNode* node = treeMap.lookup(element.id);

        GuiTreeNode* oldParent = treeMap.lookup(node->parent);
        // when parent is null, we don't need to remove element from parent
        if (oldParent) {
            for (int c = 0; c < oldParent->childCount; c++) {
                if (oldParent->children[c] == element.id) {
                    if (c != oldParent->childCount-1)
                        oldParent->children[c] = oldParent->children[oldParent->childCount-1];
                    oldParent->childCount--;
                    break;
                }
            }
            LogError("Tree node parent doesnt have node in children list!");
        }

        treeMap.remove(element.id);
    }

    // make the child element a sub element of the parent. If the child already had a parent prior to this, the parent will be replaced
    void adopt(Element parent, Element child) {
        if (parent == child) {
            LogError("Element cannot adopt itself!");
            return;
        }

        GuiTreeNode* parentNode = treeMap.lookup(parent.id);
        if (!parentNode) {
            LogError("Failed to adopt: parent is not in tree!");
            return;
        }
        // for testing
        GuiTreeNode* childNode = treeMap.lookup(child.id);
        if (childNode) {
            GuiTreeNode* oldParent = treeMap.lookup(childNode->parent);
            if (oldParent->e == parent) {
                // already has correct parent, nothing needs to be done.
                // waste of time though...
                LogWarn("Tried to adopt an element that was already a child.");
                assert(childNode->parent == parent.id);
                return;
            } else {
                // remove child from old parent
                for (int c = 0; c < oldParent->childCount; c++) {
                    if (oldParent->children[c] == child.id) {
                        if (c != oldParent->childCount-1)
                            oldParent->children[c] = oldParent->children[oldParent->childCount-1];
                        oldParent->childCount--;
                        break;
                    }
                }
                // swap parent
                childNode->parent = parent.id;
            }
        } else {
            treeMap.insert(child.id, GuiTreeNode(child, parent.id));
        }

        addChildNode(parentNode, child.id);
    }

    Element newElement(ECS::PrototypeID prototype, Element parent = NullElement) {
        Element element = ECS::EntityManager::createEntity(prototype);
        if (parent != NullElement) {
            adopt(parent, element);
        }
        return element;
    }

    void hideElement(Element element) {
        if (!entityExists(element)) {
            LogError("Element does not exist!");
            return;
        }

        hideElement(element.id);
    }

private:
    void hideElement(ElementID elementID) {
        GuiTreeNode* node = treeMap.lookup(elementID);
        if (node && !entityHas<EC::Hidden>(node->e)) {
            addComponent<EC::Hidden>(node->e, {});
            for (int i = 0; i < node->childCount; i++) {
                hideElement(node->children[i]);
            }
        }
    }
public:
    void unhideElement(Element element) {
        if (!entityExists(element)) {
            LogError("Element does not exist!");
            return;
        }

        unhideElement(element.id);
    }
private:
    void unhideElement(ElementID elementID) {
        GuiTreeNode* node = treeMap.lookup(elementID);
        if (node && entityHas<EC::Hidden>(node->e)) {
            removeComponent<EC::Hidden>(node->e);
            for (int i = 0; i < node->childCount; i++) {
                unhideElement(node->children[i]);
            }
        }
    }
public:

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

    Element getNamedElement(const char* name) {
        Element namedElement = NullElement;
        
        forEachEntity<EC::Name>([&](Element e){
            auto* nameEc = getComponent<EC::Name>(e);
            if (My::streq(nameEc->name, name)) {
                namedElement = e;
            }
        });
        return namedElement;
    }

    Element getNamedElement(std::string name) {
        return getNamedElement(name.c_str());
    }

    void destroy() {
        destroyTree(getTreeNode(screen.id));

        ECS::EntityManager::destroy();

        ECS::Systems::cleanupSystems(systemManager);
    }
};


}

#endif