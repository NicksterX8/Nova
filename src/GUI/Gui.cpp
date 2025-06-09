#include "GUI/Gui.hpp"
#include "rendering/gui.hpp"
#include "rendering/drawing.hpp"

/*

void Draw::itemStack(SDL_Renderer* renderer, float scale, ItemStack item, SDL_Rect* destination) {
    // icon
    SDL_RenderCopy(renderer, ItemData[item.item].icon, NULL, destination);
    // quantity count
    // dont draw item count over items that can only ever have one count,
    // its pointless
    if (ItemData[item.item].stackSize != 1 && item.quantity != ItemQuantityInfinity) {
        FC_DrawScale(FreeSans, renderer, destination->x + 3, destination->y - 4, FC_MakeScale(scale/2.0f,scale/2.0f),
        "%d", item.quantity);
    }
}

*/

namespace GUI {

void Gui::drawHeldItemStack(GuiRenderer& renderer, const ItemManager& itemManager, const ItemStack& itemStack, Vec2 pos) {
    if (itemStack.empty()) return;
    const float size = renderer.pixels(60);
    Vec2 bottomLeft = {
        pos.x - size/2,
        pos.y - size/2
    };
    FRect destination = {
        bottomLeft.x,
        bottomLeft.y,
        size,
        size
    };
    Draw::drawItemStack(renderer, itemManager, itemStack, destination);
    float textScale = renderer.options.scale/2.0f;
    pos = Vec2{bottomLeft.x, bottomLeft.y + Fonts->get("Gui")->descender() * textScale} + renderer.pixels({2.5f, 5.0f});
    if (items::getStackSize(itemStack.item, itemManager) != 1) {
    renderer.text->render(string_format("%d", itemStack.quantity).c_str(), pos,
        TextFormattingSettings{
            .align = TextAlignment::BottomLeft}, 
        TextRenderingSettings{
            .font = Fonts->get("Gui"), .color = {0,0,0,255}, .scale = Vec2(textScale)});
    }
}

void buildFromTree(GuiTreeNode* node, GUI::GuiManager& gui, int level) {
    Element parent = node->e;
    if (parent.Null()) return;
    auto* parentViewEc = gui.getComponent<EC::ViewBox>(parent);
    if (!parentViewEc) return;
    
    Box parentBox = parentViewEc->absolute;
    auto* fitConstraint = gui.getComponent<EC::StackConstraint>(parent);
    Box fitBox = parentBox;
    
    for (int i = 0; i < node->childCount; i++) {
        GuiTreeNode* childNode = gui.getTreeNode(node->children[i]);
        Element e = childNode->e;
        auto* viewEc = gui.getComponent<EC::ViewBox>(e);
        if (viewEc && !gui.elementHas<EC::Hidden>(e)) {
            Box childBox = viewEc->box;
            
            // size
            auto* sizeConstraint = gui.getComponent<EC::SizeConstraint>(e);
            if (sizeConstraint) {
                if (sizeConstraint->relativeSize.x != INFINITY) {
                    childBox.size.x = MIN(sizeConstraint->relativeSize.x * parentBox.size.x, fitBox.size.x);
                }
                if (sizeConstraint->relativeSize.y != INFINITY) {
                    childBox.size.y = MIN(sizeConstraint->relativeSize.y * parentBox.size.y, fitBox.size.y);
                }
                childBox.size.x = MIN(childBox.size.x, sizeConstraint->maxSize.x);
                childBox.size.y = MIN(childBox.size.y, sizeConstraint->maxSize.y);

                childBox.size.x = MAX(childBox.size.x, sizeConstraint->minSize.x);
                childBox.size.y = MAX(childBox.size.y, sizeConstraint->minSize.y);
            }
            childBox.size.x = MIN(childBox.size.x, parentBox.size.x);
            childBox.size.y = MIN(childBox.size.y, parentBox.size.y);
            if (fitConstraint)
                fitBox.size -= childBox.size * Vec2(fitConstraint->horizontal, fitConstraint->vertical);

            auto* alignmentConstraint = gui.getComponent<EC::AlignmentConstraint>(e);
            Vec2 offset = {0, 0};
            if (alignmentConstraint && !fitConstraint) {
                Vec2 margin = parentBox.size - childBox.size;
                offset.x = (int)alignmentConstraint->alignment.horizontal * 0.5f * margin.x;
                offset.y = (2 - (int)alignmentConstraint->alignment.vertical) * 0.5f * margin.y;
                childBox.min = offset;
            }
            childBox.min += fitBox.min;
            if (fitConstraint)
                fitBox.min += childBox.size * Vec2(fitConstraint->horizontal, fitConstraint->vertical);
            
            viewEc->level = level;
            viewEc->absolute = childBox;
        }

        buildFromTree(childNode, gui, ++level);
    }
}

void Gui::renderElements(GuiRenderer& renderer, const PlayerControls& playerControls) {
    Vec2 mousePos = playerControls.mousePixelPos();
    bool mouseLeftDown = playerControls.mouse.leftButtonDown();
    bool mouseClicked = playerControls.mouseClicked;

    GuiManager& gui = manager;

    /*
    gui.forEachElement([&](auto signature){
        return signature[EC::ViewBox::ID] && !signature[EC::ChildOf];
    }, [&](Element e){
        auto boxEc = gui.getComponent<EC::ViewBox>(e);

        boxEc->absolute = boxEc->box;
        boxEc->level = 0;
    });
    */

    auto screenBox = Box{Vec2(0), renderer.options.size};
    gui.setComponent(gui.screen, EC::ViewBox{screenBox, screenBox});

    int level = 1;
    buildFromTree(gui.getTreeNode(gui.screen.id), gui, level);

    /*
    gui.forEachElement<EC::ViewBox, EC::SizeConstraint>([&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        auto* sizeConstraint = gui.getComponent<EC::SizeConstraint>(e);
        Element parent = gui.getComponent<EC::ChildOf>(e)->parent;
        auto* parentBox = gui.getComponent<EC::ViewBox>(parent);

        viewbox->box.size = sizeConstraint->relativeSize * parentBox->box.size;
    });
    */


    int hoveredLevel = -1;
    Element hoveredElement = NullElement;

    gui.forEachElement([&](auto signature){ 
        return signature[EC::ViewBox::ID] && !signature[EC::Hidden::ID];
    }, [&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        if (viewbox->visible) {
            bool mouseOnButton = pointInRect(mousePos, viewbox->absolute.rect());
            if (mouseOnButton) {
                if (viewbox->level > hoveredLevel) {
                    hoveredLevel = viewbox->level;
                    hoveredElement = e;
                }
            }
        }
    });

    if (hoveredLevel >= 0) {
        gui.hoveredElement = hoveredElement;
    } else {
        gui.hoveredElement = NullElement;
    }

    executeSystems(gui.systemManager, gui);

    /* Elements are rendered based on their level
     * Parts of elements are rendered according to the order the forEach calls are made here
     */

    // text
    gui.forEachElement<EC::ViewBox, EC::Text>([&](Element e){
        if (gui.elementHas<EC::Hidden>(e)) return;

        auto* view = gui.getComponent<EC::ViewBox>(e);
        Box entityBox = view->absolute;
        auto* textComponent = gui.getComponent<EC::Text>(e);
        Vec2 pos;
        switch (textComponent->formatSettings.align.vertical) {
        case VertAlignment::Bottom:
            pos.y = entityBox.min.y;
            break;
        case VertAlignment::Middle:
            pos.y = entityBox.min.y + entityBox.size.y / 2.0f;
            break;
        case VertAlignment::Top:
            pos.y = entityBox.min.y + entityBox.size.y;
            break;
        }
        switch (textComponent->formatSettings.align.horizontal) {
        case HoriAlignment::Left:
            pos.x = entityBox.min.x;
            break;
        case HoriAlignment::Center:
            pos.x = entityBox.min.x + entityBox.size.x / 2.0f;
            break;
        case HoriAlignment::Right:
            pos.x = entityBox.min.x + entityBox.size.x;
            break;
        }

        entityBox.min = pos;
        
        renderer.boxedText(textComponent->text, entityBox, textComponent->formatSettings, textComponent->renderSettings, view->level);
    });
}

}
