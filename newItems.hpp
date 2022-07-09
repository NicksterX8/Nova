#include <SDL2/SDL.h>

typedef Uint16 ItemID;

struct ItemStack {
    ItemID id;
    Uint32 quantity;
    char data[18];
};

