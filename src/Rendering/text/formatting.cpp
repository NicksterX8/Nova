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

// returns number visible characters outputted
// charsOut must have atleast enough space to fit strlen(text) chars
int parseTextToWords(const Font* font, const Char* text, int textLength, llvm::SmallVectorImpl<Word>* wordsOut, Char* charsOut) {
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

// returns number visible characters outputted
// charsOut must have atleast enough space to fit strlen(text) chars
int parseTextToWordsWrap(const Font* font, const Char* text, int textLength, llvm::SmallVectorImpl<Word>* wordsOut, Char* charsOut) {
    assert(text);
    
    const float* advances = font->characters->advances;
    const glm::u16vec2* sizes = font->characters->sizes;
    CharIndex charCount = 0;
    for (CharIndex c = 0; c < textLength; c++) {
        Char ch = text[c];
        if (!font->hasChar(ch)) {
            ch = UnsupportedChar;
        }

        float advance = advances[ch];
        if (isWhitespace(text[c])) {
            Word word{c, 0, advance, 0.0f, ch == '\n'};
            wordsOut->push_back(word);
            continue;
        }
        
        float size = (float)sizes[ch].x;
        Word word{c, 1, advance, size, ch == '\n'};
        wordsOut->push_back(word);
        charsOut[charCount++] = ch;
    }

    return charCount;
}

struct Line {
    int startWord;
    int wordCount;
    float width;
};

// returns the bounding size of the resulting lines
Vec2 parseWordsToLines(const Font* font, float maxWidth, ArrayRef<Word> words, llvm::SmallVectorImpl<Line>* linesOut) {
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
    int charCount;
    if (formatting.wrapOnWhitespace)
        charCount = parseTextToWords(font, text, textLength, &words, charsOut->require(textLength));
    else
        charCount = parseTextToWordsWrap(font, text, textLength, &words, charsOut->require(textLength));
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