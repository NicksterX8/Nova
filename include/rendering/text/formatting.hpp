#ifndef TEXT_FORMATTING_INCLUDED
#define TEXT_FORMATTING_INCLUDED

#include "Font.hpp"

enum class HoriAlignment : Uint16 {
    Left=0,
    Center=1,
    Right=2
};
enum class VertAlignment : Uint16 {
    Top=0,
    Middle=1,
    Bottom=2
};

struct TextAlignment {
    HoriAlignment horizontal;
    VertAlignment vertical;

    #define ALIGNMENT_CONST_DECL const static TextAlignment

    ALIGNMENT_CONST_DECL TopLeft;
    ALIGNMENT_CONST_DECL TopCenter;
    ALIGNMENT_CONST_DECL TopRight;
    ALIGNMENT_CONST_DECL MiddleLeft;
    ALIGNMENT_CONST_DECL MiddleCenter;
    ALIGNMENT_CONST_DECL MiddleRight;
    ALIGNMENT_CONST_DECL BottomLeft;
    ALIGNMENT_CONST_DECL BottomCenter;
    ALIGNMENT_CONST_DECL BottomRight;

    #undef ALIGNMENT_CONST_DECL
};

inline Vec2 getAlignmentOffset(TextAlignment alignment, Vec2 size) {
    return {(float)alignment.horizontal * 0.5f * size.x, (2 - (float)alignment.vertical) * 0.5f * size.y};
}

// handle horizontal alignment by shifting line to correct position on the x axis
inline void alignLineHorizontally(MutableArrayRef<glm::vec2> offsets, float originOffset, float paragraphWidth, HoriAlignment align) {
    if (align != HoriAlignment::Left) {
        float width = originOffset;
        // shift whole line over to correct alignment location (center or right)
        float lineMovement = 0.0f;
        if (align == HoriAlignment::Right) {
            lineMovement = 1.0f * width;
        } else if (align == HoriAlignment::Center) {
            lineMovement = 0.5f * width;
        }
        for (int j = 0; j < offsets.size(); j++) {
            offsets[j].x -= lineMovement;
        }
    }
}

struct TextFormattingSettings {
    TextAlignment align = TextAlignment::TopLeft;
    float maxWidth = INFINITY;
    float maxHeight = INFINITY;
    float sizeOffsetScale = 0.0f; // never actually used currently. maybe remove??
    bool wrapOnWhitespace = true;

private:
    using Self = TextFormattingSettings;
public:

    static Self Default() {
        return Self{};
    }
};

struct CharacterLayoutData {
    glm::vec2 size = {0, 0}; // size of the body of output text
    glm::vec2 origin = {0, 0};
    glm::vec2 offset = {0, 0}; // offset to original position
    llvm::SmallVector<char> characters;
    llvm::SmallVector<glm::vec2> characterOffsets;
    llvm::SmallVector<char> whitespaceCharacters;
    llvm::SmallVector<glm::vec2> whitespaceCharacterOffsets;
};

CharacterLayoutData formatText(const Font* font, const char* text, int textLength, TextFormattingSettings settings, glm::vec2 scale);

#endif