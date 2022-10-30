#include "GUI/Gui.hpp"

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