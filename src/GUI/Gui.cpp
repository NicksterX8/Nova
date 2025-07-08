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

void Gui::drawConsole(GuiRenderer& renderer) {
    Element consoleElement = manager.getNamedElement("console");
    auto* consoleView = manager.getComponent<EC::ViewBox>(consoleElement);

    float scale = renderer.options.scale;
    bool showLog = console.promptOpen || Metadata->seconds() - console.timeLastMessageSent < CONSOLE_LOG_NEW_MESSAGE_OPEN_DURATION;
    if (!showLog && console.activeMessage.empty()) {
        manager.hideElement(consoleElement);
        return;
    } else {
        manager.unhideElement(consoleElement);
    }
    
    Element consoleLogElement = manager.getNamedElement("console-log");
    auto* logViewEc = manager.getComponent<EC::ViewBox>(consoleLogElement);
    if (showLog && logViewEc) {
        manager.unhideElement(consoleElement);
        
        TextFormattingSettings logFormatting{
            .align = TextAlignment::BottomLeft,
            .maxWidth = ConsoleLogWidth,
            .wrapOnWhitespace = false
        };
        TextRenderingSettings logRenderingSettings {
            .font = Fonts->get("Debug"),
            .scale = 0.5f
        };
        float messageSpacing = logRenderingSettings.font->height() * 0.25f;
        Vec2 pos = logViewEc->absolute.min;
        for (int i = console.log.size()-1; i >= 0; i--) {
            const Console::LogMessage& message = console.log[i];
            Console::Text text = console.getText(message);
            
            TextRenderer::RenderResult result;
            if (text.colors.size() == 1) {
                logRenderingSettings.color = message.colors.getSingleValue();
                result = renderer.renderText(text.text.c_str(), pos, logFormatting, logRenderingSettings, logViewEc->level);
            } else {
                result = renderer.renderColoredText(text.text.c_str(), text.colors, pos, logFormatting, logRenderingSettings, logViewEc->level);
            }
            pos.y += result.rect.h + messageSpacing;
            // check if message will be visible on screen
            if (pos.y >= renderer.options.size.y) break;
        }
    } else {
        manager.hideElement(consoleLogElement);
    }
    
    SDL_Color terminalTextColor = {255,255,255,255};

    Element consoleTerminal = manager.getNamedElement("console-terminal");
    EC::ViewBox* terminalViewEc = manager.getComponent<EC::ViewBox>(consoleTerminal);
    const std::string& activeMessage = console.activeMessage;

    TextFormattingSettings terminalTextFormatting{
        .align = TextAlignment::BottomLeft,
        .maxWidth = renderer.options.size.x,
        .wrapOnWhitespace = false
    };
    const Font* terminalFont = Fonts->get("Debug");
    float terminalFontScale = 0.75;
    TextRenderingSettings terminalTextRenderSettings{
        .color = terminalTextColor,
        .scale = terminalFontScale,
        .font = terminalFont
    };

    bool showTerminal = console.promptOpen;
    if (showTerminal) {
        manager.unhideElement(consoleTerminal);

        // TODO: reimplement cursor
        Vec2* characterPositions = nullptr;
        auto textRect = renderer.renderText(activeMessage.c_str(), terminalViewEc->absolute.min, terminalTextFormatting, terminalTextRenderSettings, terminalViewEc->level).rect;
        terminalViewEc->box = *rectAsBox(&textRect);
        if (terminalViewEc->box.size.y < terminalFont->height()) {
            terminalViewEc->box.size.y = terminalFont->height();
        }

        // Vec2 selectedCharPos = terminalViewEc->absolute.min;
        // int selectedCharIndex = console.selectedCharIndex;
        
        // // in the middle case
        // if (selectedCharIndex >= 0 && selectedCharIndex < activeMessage.size()) {
        //     selectedCharPos = characterPositions[selectedCharIndex];
        //     if (selectedCharIndex > 0) {
        //         selectedCharPos = characterPositions[selectedCharIndex-1];
        //         selectedCharPos.x += terminalFont->advance(activeMessage.back()) * terminalFontScale;
        //     }
        // // end case
        // } else if (activeMessage.size() > 0 && selectedCharIndex == activeMessage.size()) {
        //     selectedCharPos = characterPositions[activeMessage.size()-1];
        //     selectedCharPos.x += terminalFont->advance(activeMessage.back()) * terminalFontScale;
        // }
        // // for case where message is empty
        // if (activeMessage.size() > 0) {
        //     selectedCharPos.y += terminalFont->descender() * terminalFontScale;
        // }

        // Box cursor = {
        //     selectedCharPos,
        //     {renderer.pixels(2.0f), terminalFont->height() * terminalFontScale}
        // };
        
        // // render cursor
        // constexpr double flashDelay = 0.5;
        // static double timeTilFlash = flashDelay;
        // constexpr double flashDuration = 0.5;
        // timeTilFlash -= Metadata->frame.deltaTime / 1000.0; // subtract time in seconds from time
        // double secondsSinceCursorMove = Metadata->seconds() - console.timeLastCursorMove;
        // if (secondsSinceCursorMove < flashDuration) timeTilFlash = -secondsSinceCursorMove;
        // if (timeTilFlash < 0.0) {
        //     renderer.colorRect(cursor, terminalTextColor, terminalViewEc->level);
        // }
    } else {
        manager.hideElement(consoleTerminal);
    }
}

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
            .font = Fonts->get("Gui"), .color = {0,0,0,255}, .scale = textScale});
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
        if (viewEc && !gui.entityHas<EC::Hidden>(e)) {
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
                offset = getAlignmentOffset(alignmentConstraint->alignment, margin);
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

    gui.forEachEntity([&](auto signature){ 
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

    executeSystems(gui.systemManager);

    /* Elements are rendered based on their level
     * Parts of elements are rendered according to the order the forEach calls are made here
     */

    // text
    gui.forEachEntity<EC::ViewBox, EC::Text>([&](Element e){
        if (gui.entityHas<EC::Hidden>(e)) return;

        auto* view = gui.getComponent<EC::ViewBox>(e);
        Box entityBox = view->absolute;
        auto* textComponent = gui.getComponent<EC::Text>(e);

        Vec2 alignmentOffset = Text::getAlignmentOffset(textComponent->formatSettings.align, entityBox.size);
        Vec2 pos = entityBox.min + alignmentOffset;

        entityBox.min = pos;
        
        renderer.boxedText(textComponent->text, entityBox, textComponent->formatSettings, textComponent->renderSettings, view->level);
    });
}

}
