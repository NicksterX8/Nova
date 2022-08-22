#ifndef GAMEVIEWPORT_INCLUDED
#define GAMEVIEWPORT_INCLUDED

#include "NC/cpp-vectors.hpp"
#include "constants.hpp"

struct GameViewport {
    Rect display;
    FRect world;

    GameViewport();
    GameViewport(Rect displayViewport, FRect worldViewport);

    void scaleWorldView(float scale);

    /*
    * The ratio or scale of world units to display units in the viewport,
    * for example if the display width is 300 pixels and the world viewport width is 30,
    * the x return component will be 10, representing that one world unit is 10 pixels wide.
    * This should be equal to TilePixels, and is generally preferred.
    */
    Vec2 worldDisplayScale() const;

    /*
    * The ratio or scale of world units to display units in the viewport,
    * for example if the display width is 300 pixels and the world viewport width is 30,
    * the x return component will be 10, representing that one world unit is 10 pixels wide.
    */
    float worldDisplayScaleX() const;
    /*
    * The ratio or scale of world units to display units in the viewport,
    * for example if the display width is 300 pixels and the world viewport width is 30,
    * the x return component will be 10, representing that one world unit is 10 pixels wide.
    */
    float worldDisplayScaleY() const;

    /*
    * Get the world position in the same location as the display relative coordinates.
    * inputs < 0 or > display width will work, with the coordinates being scaled according to
    * the scale of display to world viewport size.
    * @param displayX an offset from the left side of the display viewport's position
    * @param displayY an offset from the top side of the display viewport's position
    */
    Vec2 displayToWorldPosition(int displayX, int displayY) const;

    /*
    * Get the world position in the same location as the screen relative pixel coordinates.
    * inputs < 0 or > display width will work, with the coordinates being scaled according to
    * the scale of display to world viewport size.
    * @param pixelX an offset from the left side of the screen pixel position
    * @param pixelY an offset from the top side of the screen pixel position
    */
    Vec2 pixelToWorldPosition(int pixelX, int pixelY) const;

    /*
    * Get the world position in the same location as the screen relative pixel coordinates.
    * inputs < 0 or > display width will work, with the coordinates being scaled according to
    * the scale of display to world viewport size.
    * @param pixelX an offset from the left side of the screen pixel position
    * @param pixelY an offset from the top side of the screen pixel position
    */
    Vec2 pixelToWorldPosition(IVec2 pixelPos) const;

    /*
    * Get the display position in the same location as where the world coordinates would be on the display,
    * assuming display.x and display.y are both 0. For factoring those in, use worldToPixelPosition.
    * Keep in mind that not all world coordinates are within the display,
    * meaning it is possible for display coordinates to be < 0 or > display viewport width.
    * @param worldX a world coordinate
    * @param displayY a world coordinate
    */
    IVec2 worldToDisplayPosition(float worldX, float worldY) const;

    /*
    * Get the display position in the same location as where the world coordinates would be on the display,
    * assuming display.x and display.y are both 0. For factoring those in, use worldToPixelPosition.
    * Keep in mind that not all world coordinates are within the display,
    * meaning it is possible for display coordinates to be < 0 or > display viewport width.
    * @param position a world coordinate
    */
    IVec2 worldToDisplayPosition(Vec2 position) const;

    /*
    * Get the display position in the same location as where the world coordinates would be on the display,
    * with display x and y offsets factored in.
    * Keep in mind that not all world coordinates are within the display,
    * meaning it is possible for display coordinates to be < 0 or > display viewport width.
    * @param worldX a world coordinate
    * @param displayY a world coordinate
    */
    IVec2 worldToPixelPosition(float worldX, float worldY) const;

    /*
    * Get the display position in the same location as where the world coordinates would be on the display in float precision,
    * with display x and y offsets factored in.
    * Keep in mind that not all world coordinates are within the display,
    * meaning it is possible for display coordinates to be < 0 or > display viewport width.
    * @param worldX a world coordinate
    * @param displayY a world coordinate
    */
    Vec2 worldToPixelPositionF(Vec2 position) const;

    int displayRightEdge() const;

    int displayBottomEdge() const;

    float worldRightEdge() const;

    float worldBottomEdge() const;
};

#endif