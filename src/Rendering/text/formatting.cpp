#include "rendering/text/formatting.hpp"

const TextAlignment TextAlignment::TopLeft      = {HoriAlignment::Left,   VertAlignment::Top};
const TextAlignment TextAlignment::TopCenter    = {HoriAlignment::Center, VertAlignment::Top};
const TextAlignment TextAlignment::TopRight     = {HoriAlignment::Right,  VertAlignment::Top};
const TextAlignment TextAlignment::MiddleLeft   = {HoriAlignment::Left,   VertAlignment::Middle};
const TextAlignment TextAlignment::MiddleCenter = {HoriAlignment::Center, VertAlignment::Middle};
const TextAlignment TextAlignment::MiddleRight  = {HoriAlignment::Right,  VertAlignment::Middle};
const TextAlignment TextAlignment::BottomLeft   = {HoriAlignment::Left,   VertAlignment::Bottom};
const TextAlignment TextAlignment::BottomCenter = {HoriAlignment::Center, VertAlignment::Bottom};
const TextAlignment TextAlignment::BottomRight  = {HoriAlignment::Right,  VertAlignment::Bottom};

CharacterLayoutData formatText(const Font* font, const char* text, int textLength, TextFormattingSettings settings, glm::vec2 scale) {
    if (textLength == 0) return {};

    // all coordinates used and returned here are the unscaled versions; they're scaled later (in render, or shader)
    const float height = (float)font->height();
    const float ascender = (float)font->ascender();
    const float descender = (float)font->descender();
    const float lineSpacing = font->formatting.lineSpacing * height;

    glm::vec2 maxSize = glm::vec2{settings.maxWidth, settings.maxHeight} / scale;

    float cx = 0.0f, mx = 0.0f; // current x, max x
    float y = 0.0f; // y
    int lineLength = 0;
    int wordLength = 0;
    int lineCount = 1;

    CharacterLayoutData layout = {};

    llvm::SmallVector< std::array<int, 2> > lines;
    llvm::SmallVector<float> lineSizes;
    int lineBreakIndex = -1; // need to put a break before this character

    for (int i = 0; i < textLength; i++) {
        char c = text[i];
    
        // FT_Vector fractionalKerning = {0,0}; 
        // auto leftCharIndex = FT_Get_Char_Index(font->face, c);
        // auto rightCharIndex = FT_Get_Char_Index(font->face, lastChar);
        // FT_Get_Kerning(font->face, leftCharIndex, rightCharIndex, FT_KERNING_UNFITTED, &fractionalKerning);

        // glm::vec2 kerning = {fractionalKerning.x / 64.0f * scale.x, fractionalKerning.y / 64.0f * scale.y};
        // cx += kerning.x;

        bool whitespaceChar = isWhitespace(c); // current character is whitespace (nothing to render, does nothing but shift formatting)
        
        if (!whitespaceChar && (c < ASCII_FIRST_STANDARD_CHAR || c > ASCII_LAST_STANDARD_CHAR)) {
            // turn unsupported characters into hashtags
            c = '#';
        }

        float charPixelsAdvance = (float)font->advance(c);
        
        bool timeForNewLine = cx + font->size(c).x >= maxSize.x;

        if (timeForNewLine) {
            if (settings.wrapOnWhitespace && lineBreakIndex == -1) {
                if (whitespaceChar) {
                    lineBreakIndex = layout.characters.size();
                } else {
                    lineBreakIndex = layout.characters.size() - wordLength;
                }
            }
        }
            
        if (c == '\n' ||
            (timeForNewLine &&
            (!settings.wrapOnWhitespace || whitespaceChar )))
        {
            if (wordLength != i) 
                y -= lineSpacing;
            if (y > maxSize.y) {
                // no room for a new line. stop processing text
                break;
            }

            // wrap on whitespace, shift words together
            // float wrapOffsetX = 0.0f;
            // if (c != '\n' && settings.wrapOnWhitespace && wordLength > 0 && wordLength < lineLength) {
            //     // index through the last wordLength characters
            //     // should be impossible for layout.characters.size to be less than wordLength
            //     // in the case that a word is so long that it can't even fit on line by itself
            //     //  (the word length is > maxSize.x), the word would be wrapped onto two lines,
            //     //  so the word length would be larger than the line length. That's the reason
            //     //  taking the minimum is necessary here, so we don't shift the line above the
            //     //  current one, which has already been shifted and is in correct position.
            //     int segmentLength = wordLength;
            //     int startIndex = layout.characters.size() - segmentLength;
            //     wrapOffsetX = layout.characterOffsets[startIndex].x;
            //     for (int j = 0; j < segmentLength; j++) {
            //         auto* characterOffset = &layout.characterOffsets[startIndex + j];
            //         characterOffset->x -= wrapOffsetX;
            //         characterOffset->y = y;
            //     }
            //     //lineLength = MAX(wordLength, lineLength);
            // }

            //float lineSize = cx;

            // all characters that are 'cheating' over the line that we waited to do until whitespace
            float wrapOffsetX = 0.0f;
            if (lineBreakIndex > 0 && lineBreakIndex < layout.characterOffsets.size()) {
                int startIndex = lineBreakIndex;
                int numMovedChars = layout.characterOffsets.size() - lineBreakIndex;
                wrapOffsetX = layout.characterOffsets[startIndex].x;
                for (int j = startIndex; j < layout.characterOffsets.size(); j++) {
                    // move all those characters down onto next line
                    layout.characterOffsets[j].y = y;
                    layout.characterOffsets[j].x -= wrapOffsetX;
                }

                //auto line = MutableArrayRef<glm::vec2>(layout.characterOffsets.end() - lineLength - numMovedChars, layout.characterOffsets.end() - numMovedChars);
                
                if (layout.characterOffsets.size() > numMovedChars) {
                    lines.push_back({(int)layout.characterOffsets.size() - lineLength, (int)layout.characterOffsets.size() - numMovedChars});
                    float lineSize = layout.characterOffsets[lines.back()[1]-1].x - layout.characterOffsets[lines.back()[0]].x + font->size(layout.characters[lines.back()[1]-1]).x;
                    lineSizes.push_back(lineSize);
                }

                lineLength = layout.characterOffsets.size() - lineBreakIndex;
                cx -= wrapOffsetX;
                if (c == '\n') {
                    y -= lineSpacing;
                    if (y > maxSize.y) {
                        // no room for a new line. stop processing text
                        break;
                    }

                    cx = 0.0f;
                    lines.push_back({(int)layout.characterOffsets.size() - lineLength, (int)layout.characterOffsets.size()});
                    float lineSize = layout.characterOffsets[lines.back()[1]-1].x - layout.characterOffsets[lines.back()[0]].x + font->size(layout.characters[lines.back()[1]-1]).x;
                    lineSizes.push_back(lineSize);
                    lineLength = 0;
                }
            } else {
                cx = 0.0f;
                if (lineLength > 0) {
                    lines.push_back({(int)layout.characterOffsets.size() - lineLength, (int)layout.characterOffsets.size()});
                    float lineSize = layout.characterOffsets[lines.back()[1]-1].x - layout.characterOffsets[lines.back()[0]].x + font->size(layout.characters[lines.back()[1]-1]).x;
                    lineSizes.push_back(lineSize);
                    lineLength = 0;
                }
            }

            lineBreakIndex = -1;

            // reset line
            mx = MIN(MAX(mx, cx), maxSize.x);
            lineCount++;
        }

        if (true) {
            if (!whitespaceChar) {
                layout.characters.push_back(c);
                layout.characterOffsets.push_back(glm::vec2(cx, y));
                lineLength++;
                wordLength++;
            } else {
                layout.whitespaceCharacters.push_back(c);
                layout.whitespaceCharacterOffsets.push_back(glm::vec2(cx, y));
                wordLength = 0;
            }
            cx += charPixelsAdvance;
        }
    }

    mx = cx > mx ? cx : mx;


    lines.push_back({(int)layout.characterOffsets.size() - lineLength, (int)layout.characterOffsets.size()});
    lineSizes.push_back(cx);

    for (int i = 0; i < lines.size(); i++) {
        auto line = MutableArrayRef<glm::vec2>(layout.characterOffsets.data() + lines[i][0], layout.characterOffsets.data() + lines[i][1]);
        alignLineHorizontally(line, lineSizes[i], mx, settings.align.horizontal);
    }

    glm::vec2 size = {mx, (lineCount-1) * (lineSpacing) + height};
    glm::vec2 origin = {0, 0};
    glm::vec2 offset = {0, 0};
    
    switch (settings.align.vertical) {
    case VertAlignment::Top:
        break;
    case VertAlignment::Middle:
        offset.y = size.y / 2.0f;
        break;
    case VertAlignment::Bottom:
        offset.y = size.y;
        break;
    default:
        break;
    }

    origin.y = -offset.y + ascender;

    // pos + offset = top left corner

    layout.size = size;
    layout.origin = origin;
    layout.offset = offset;
    return layout;
}