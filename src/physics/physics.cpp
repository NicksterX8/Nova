#include "utils/vectors_and_rects.hpp"
#include "utils/common-macros.hpp"
#include <iostream>

bool boxesOverlap(const Boxf* ba, const Boxf* bb) {
    return (
        (*ba)[1].x > (*bb)[0].x &&
        (*ba)[1].y > (*bb)[0].y &&
        (*ba)[0].x < (*bb)[1].x &&
        (*ba)[0].y < (*bb)[1].y );
}

struct Rectangle {
    int x1,y1,x2,y2;
};

// chatgpt
Rectangle calculateOverlapRectangle(const Rectangle& rect1, const Rectangle& rect2) {
    Rectangle overlapRect;

   // Calculate the overlapping rectangle
    overlapRect.x2 = rect1.x2 ^ ((rect1.x2 ^ rect2.x2) & -(rect1.x2 > rect2.x2));
    overlapRect.y2 = rect1.y2 ^ ((rect1.y2 ^ rect2.y2) & -(rect1.y2 > rect2.y2));
    overlapRect.x1 = rect1.x1 ^ ((rect1.x1 ^ rect2.x1) & -(rect1.x1 < rect2.x1));
    overlapRect.y1 = rect1.y1 ^ ((rect1.y1 ^ rect2.y1) & -(rect1.y1 < rect2.y1));
    //overlapRect.x1 = overlapRect.x1 & (overlapRect.x1 - overlapRect.x2);

    // Ensure the rectangles actually overlap
    if (UNLIKELY(overlapRect.x1 >= overlapRect.x2 || overlapRect.y1 >= overlapRect.y2)) {
        // No overlap, return a default rectangle with zero area
        overlapRect = {0, 0, 0, 0};
    }

    return overlapRect;
}

int physics_test() {
    Rectangle rect1 = {16, -5, 40, 20};
    Rectangle rect2 = {3, 7, 7, 3};
    
    Rectangle overlapRect = calculateOverlapRectangle(rect1, rect2);
    
    //std::cout << "Overlapping Rectangle: (" << overlapRect.x1 << ", " << overlapRect.y1 << ") to (" << overlapRect.x2 << ", " << overlapRect.y2 << ")" << std::endl;
    
    return 0;
}