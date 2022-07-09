#include "GameViewport.hpp"
#include "constants.hpp"

GameViewport::GameViewport() {
    display = {
        0, 0, 1, 1
    };
    world = {
        0, 0, 1, 1
    };
}
GameViewport::GameViewport(Rect displayViewport, FRect worldViewport) {
    if (displayViewport.w < 0) {
        displayViewport.w = -displayViewport.w;
    }
    if (displayViewport.h < 0) {
        displayViewport.h = -displayViewport.h;
    }
    if (worldViewport.w < 0) {
        worldViewport.w = -worldViewport.w;
    }
    if (worldViewport.h < 0) {
        worldViewport.h = -worldViewport.h;
    }

    if (displayViewport.w == 0) {
        displayViewport.w = 1;
    }
    if (displayViewport.h == 0) {
        displayViewport.h = 1;
    }

    if (worldViewport.w == 0) {
        worldViewport.w = 1;
    }
    if (worldViewport.h == 0) {
        worldViewport.h = 1;
    }

    display = displayViewport;
    world = worldViewport;
}

void GameViewport::scaleWorldView(float scale) {
    world.w *= scale;
    world.h *= scale;
}

/*
* The ratio or scale of world units to display units in the viewport,
* for example if the display width is 300 pixels and the world viewport width is 30,
* the x return component will be 10, representing that one world unit is 10 pixels wide.
* This should be equal to TilePixels, and is generally preferred.
*/
Vec2 GameViewport::worldDisplayScale() const {
    return {
        display.w / world.w,
        display.h / world.h
    };
}
/*
* The ratio or scale of world units to display units in the viewport,
* for example if the display width is 300 pixels and the world viewport width is 30,
* the x return component will be 10, representing that one world unit is 10 pixels wide.
*/
float GameViewport::worldDisplayScaleX() const {
    return display.w / world.w;
}
/*
* The ratio or scale of world units to display units in the viewport,
* for example if the display width is 300 pixels and the world viewport width is 30,
* the x return component will be 10, representing that one world unit is 10 pixels wide.
*/
float GameViewport::worldDisplayScaleY() const {
    return display.h / world.h;
}

/*
* Get the world position in the same location as the display relative coordinates.
* inputs < 0 or > display width will work, with the coordinates being scaled according to
* the scale of display to world viewport size.
* @param displayX an offset from the left side of the display viewport's position
* @param displayY an offset from the top side of the display viewport's position
*/
Vec2 GameViewport::displayToWorldPosition(int displayX, int displayY) const {
    return {
        ((float)displayX / display.w) * world.w + world.x,
        ((float)displayY / display.h) * world.h + world.y
    };
}

Vec2 GameViewport::pixelToWorldPosition(int pixelX, int pixelY) const {
    return {
        ((float)(pixelX - display.x) / display.w) * world.w + world.x,
        ((float)(pixelY - display.y) / display.h) * world.h + world.y
    };
}

Vec2 GameViewport::pixelToWorldPosition(IVec2 pixelPos) const {
    return {
        ((float)(pixelPos.x - display.x) / display.w) * world.w + world.x,
        ((float)(pixelPos.y - display.y) / display.h) * world.h + world.y
    };
}

IVec2 GameViewport::worldToDisplayPosition(float worldX, float worldY) const {
    return {
        (int)floor((worldX - world.x) * (display.w / world.w)),
        (int)floor((worldY - world.y) * (display.h / world.h))
    };
}

IVec2 GameViewport::worldToDisplayPosition(Vec2 position) const {
    return {
        (int)floor((position.x - world.x) * (display.w / world.w)),
        (int)floor((position.y - world.y) * (display.h / world.h))
    };
}

IVec2 GameViewport::worldToPixelPosition(float worldX, float worldY) const {
    return {
        (int)floor((worldX - world.x) * (display.w / world.w)) + display.x,
        (int)floor((worldY - world.y) * (display.h / world.h)) + display.y
    };
}

/*
* Get the display position in the same location as where the world coordinates would be on the display in float precision,
* with display x and y offsets factored in.
* Keep in mind that not all world coordinates are within the display,
* meaning it is possible for display coordinates to be < 0 or > display viewport width.
* @param worldX a world coordinate
* @param displayY a world coordinate
*/
Vec2 GameViewport::worldToPixelPositionF(Vec2 position) const {
    return {
        (position.x - world.x) * (display.w / world.w) + display.x,
        (position.y - world.y) * (display.h / world.h) + display.y
    };
}

int GameViewport::displayRightEdge() const {
    return display.w + display.x;
}

int GameViewport::displayBottomEdge() const {
    return display.h + display.y;
}

float GameViewport::worldRightEdge() const {
    return world.w + world.x;
}

float GameViewport::worldBottomEdge() const {
    return world.h + world.y;
}