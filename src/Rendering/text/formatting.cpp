#include "rendering/text/formatting.hpp"

namespace Text {

const TextAlignment TextAlignment::TopLeft       = {HoriAlignment::Left,   VertAlignment::Top};
const TextAlignment TextAlignment::TopCenter     = {HoriAlignment::Center, VertAlignment::Top};
const TextAlignment TextAlignment::TopRight      = {HoriAlignment::Right,  VertAlignment::Top};
const TextAlignment TextAlignment::MiddleLeft    = {HoriAlignment::Left,   VertAlignment::Middle};
const TextAlignment TextAlignment::MiddleCenter  = {HoriAlignment::Center, VertAlignment::Middle};
const TextAlignment TextAlignment::MiddleRight   = {HoriAlignment::Right,  VertAlignment::Middle};
const TextAlignment TextAlignment::BottomLeft    = {HoriAlignment::Left,   VertAlignment::Bottom};
const TextAlignment TextAlignment::BottomCenter  = {HoriAlignment::Center, VertAlignment::Bottom};
const TextAlignment TextAlignment::BottomRight   = {HoriAlignment::Right,  VertAlignment::Bottom};
const TextAlignment TextAlignment::JustifyBottom = {HoriAlignment::None,   VertAlignment::Bottom, true};
const TextAlignment TextAlignment::JustifyMiddle = {HoriAlignment::None,   VertAlignment::Bottom, true};
const TextAlignment TextAlignment::JustifyTop    = {HoriAlignment::None,   VertAlignment::Bottom, true};

/*
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
*/

// returns number visible characters outputted
// charsOut must have atleast enough space to fit strlen(text) chars
int parseTextToWords(const Font* font, const Char* text, int textLength, llvm::SmallVector<Word>* wordsOut, Char* charsOut) {
    assert(text);
    
    const float* advances = font->characters->advances;

    CharIndex charCount = 0;
    float wordAdvance = 0.0f;
    float wordSize = 0.0f;
    CharIndex wordLength = 0;
    for (CharIndex c = 0; c < textLength; c++) {
        Char ch = text[c];
        if (!font->hasChar(ch)) {
            ch = UnsupportedChar;
        }
        float cAdvance = advances[ch];
        if (isWhitespace(ch)) {
            Word word{(CharIndex)(c - wordLength), wordLength, wordAdvance + cAdvance, wordSize, ch == '\n'};
            wordsOut->push_back(word);
            wordLength = 0;
            wordAdvance = 0;
            wordSize = 0;
        } else {
            wordLength++;
            auto cSize = font->size(ch).x;
            wordSize = wordAdvance + cSize; // before advance is advanced
            wordAdvance += cAdvance;
            charsOut[charCount++] = ch;
        }
    }
    // get any leftover chars
    if (wordLength > 0) {
        CharIndex c = (CharIndex)textLength - 1; // last char
        Word word{(CharIndex)(textLength - wordLength), wordLength, wordAdvance, wordSize, text[c] == '\n'};
        wordsOut->push_back(word);
    }

    return charCount;
}

struct Line {
    int startWord;
    int wordCount;
    float width;
};

// returns the bounding size of the resulting lines
Vec2 parseWordsToLines(const Font* font, float maxWidth, ArrayRef<Word> words, llvm::SmallVector<Line>* linesOut) {
    assert(font && linesOut);

    if (words.empty()) return {0,0};
    
    float lineAdvance = 0.0f;
    int lineWordCount = 0;
    
    float highestWidth = 0.0f;
    bool breakAfterWord = false;
    for (int w = 0; w < words.size(); w++) {
        auto& word = words[w];
        if ((lineAdvance + word.size >= maxWidth || breakAfterWord) && lineWordCount > 0) {
            float width = lineAdvance;
            highestWidth = std::max(highestWidth, width);
            linesOut->push_back(Line{
                .startWord = w - lineWordCount,
                .wordCount = lineWordCount,
                .width = width // swap last words advance and size. we know words[w-1] exists because lineWordCount > 0
            });
            // reset line, then add word
            lineWordCount = 1;
            lineAdvance = word.advance;
        } else {
            lineWordCount++;
            lineAdvance += word.advance;
        }

        breakAfterWord = word.linebreak;
    }
    // get any left over words if they exist
    if (lineWordCount > 0) {
        float width = lineAdvance;
        highestWidth = std::max(highestWidth, width);
        linesOut->push_back(Line{
            .startWord = (int)words.size() - lineWordCount,
            .wordCount = lineWordCount,
            .width = width
        });
    }

    const float lineOffset = font->linePixelSpacing();
    const float lineHeight = font->height(); // i think?

    // line count must be > 0 if words are > 0
    float height = (linesOut->size() - 1) * lineOffset + lineHeight;
    return { highestWidth, height };
}

struct FormattingOptions {
    float lineSpacing;
    float maxWidth;
    TextAlignment align;
};

// returns bottom left corner of bounding box
void formatLines(const Font* font, const Char* text, const TextFormattingSettings& formatting,
    ArrayRef<Word> words, ArrayRef<Line> lines, Vec2 position, float scale,
    Vec2* outCharPositions)
{
    const float* advances = font->characters->advances;
    const float lineOffset = font->linePixelSpacing() * scale;
    const float lineHeight = font->height() * scale;

    auto align = formatting.align;
        
    int charIndex = 0;

    const float textHeight = (lines.size() - 1) * lineOffset + lineHeight;
    
    float vertAlignmentOffset = ((float)align.vertical) * 0.5f * textHeight - font->ascender() * scale;
    Vec2 origin = {position.x, position.y + vertAlignmentOffset};
    for (int l = 0; l < lines.size(); l++) {
        auto& line = lines[l];
        if (align.justify()) {
            // TODO: justify
            LogError("UNIMPLEMENTED: justify");
        } else {
            float horiAlignmentOffset = (float)align.horizontal * 0.5f * line.width;
            Vec2 linePos = Vec2{(origin.x) / scale - horiAlignmentOffset, (origin.y - lineOffset * l) / scale};
            float wordAdvance = 0.0f;
            for (int w = line.startWord; w < line.startWord + line.wordCount; w++) {
                auto& word = words[w];
                float charAdvance = wordAdvance;
                for (CharIndex c = word.start; c < word.end(); c++) {
                    outCharPositions[charIndex++] = Vec2{charAdvance + linePos.x, linePos.y};
                    charAdvance += advances[text[c]];
                }
                wordAdvance += word.advance;
            }
        }
    }
}

// return: output box 
FormatResult formatText(const Font* font, const Char* text, int textLength,
    const TextFormattingSettings& formatting, Vec2 position, float scale,
    My::Vec<Char>* charsOut, My::Vec<Vec2>* charPositionsOut)
{
    llvm::SmallVector<Word> words;
    int charCount = parseTextToWords(font, text, textLength, &words, charsOut->require(textLength));
    charsOut->size -= textLength - charCount;

    float unscaledMaxWidth = formatting.maxWidth / scale;
    llvm::SmallVector<Line> lines;
    Vec2 boundSize = parseWordsToLines(font, unscaledMaxWidth, words, &lines) * scale;

    formatLines(font, text, formatting, words, lines, position, scale, charPositionsOut->require(charCount));

    Vec2 bottomLeft = position - getAlignmentOffset(formatting.align, boundSize);
    return {
        Box{bottomLeft, boundSize},
        charCount
    };
}

}