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

void GUI::Gui::drawHeldItemStack(GuiRenderer& renderer, const ItemManager& itemManager, const ItemStack& itemStack, glm::vec2 pos) {
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
        TextFormattingSettings(TextAlignment::BottomLeft), TextRenderingSettings(Fonts->get("Gui"), {0,0,0,255}, Vec2(textScale)));
    }
}