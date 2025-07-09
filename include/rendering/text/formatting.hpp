#ifndef TEXT_FORMATTING_INCLUDED
#define TEXT_FORMATTING_INCLUDED

#include "Font.hpp"

namespace Text {

struct Word {
    CharIndex start;
    CharIndex length;

    float advance;
    float size;

    bool linebreak;

    Word(CharIndex startIndex, CharIndex length, float advance, float size, bool linebreak) : start(startIndex), length(length), advance(advance), size(size), linebreak(linebreak) {}

    CharIndex end() const {
        return start + length;
    }

    // does the word contain any visible characters or just whitespace
    bool visible() const {
        return length > 0;
    }
};

enum class HoriAlignment : Uint8 {
    Left=0,
    Center=1,
    Right=2,
    None = Left
};
enum class VertAlignment : Uint8 {
    Top=0,
    Middle=1,
    Bottom=2
};

struct TextAlignment {
    HoriAlignment horizontal;
    VertAlignment vertical;
    bool _justify = false;

    bool justify() const {
        return _justify;
    }

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
    ALIGNMENT_CONST_DECL JustifyTop;
    ALIGNMENT_CONST_DECL JustifyMiddle;
    ALIGNMENT_CONST_DECL JustifyBottom;

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
    SmallVector<char> characters;
    SmallVector<glm::vec2> characterOffsets;
    SmallVector<char> whitespaceCharacters;
    SmallVector<glm::vec2> whitespaceCharacterOffsets;
};

struct FormatResult {
    Box boundingBox;
    int visibleCharCount;
};

FormatResult formatText(const Font* font, const Char* text, int textLength,
    const TextFormattingSettings& formatting, Vec2 position, float scale,
    My::Vec<Char>* charsOut, My::Vec<Vec2>* charPositionsOut);

}

using Text::TextAlignment;
using Text::TextFormattingSettings;

#endif