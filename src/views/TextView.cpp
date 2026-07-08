/*
 * MIT License
 *
 * Copyright (c) 2022-2026 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TextView.hpp"
#include "CanvasView.hpp"
#include "Window.hpp"
#include "Display.hpp"
#include "Font.hpp"
#include "TextLayout.hpp"
#include "Utf8.hpp"
#include <cstdlib>

namespace {

// Internal, one-line render surface built on CanvasView's per-line state.
// Everything a line needs arrives as arguments; the LineScratch holds no
// per-line state, so lines can render independently in any order.
class LineScratch : public focus::CanvasView {
public:
    LineScratch(focus::Rect rect) : focus::CanvasView(rect) {}

    void renderLine(UNICODE_CODEPOINT* codepoints, size_t len,
                    uint8_t emphasisAtStart, bool trailingHyphen,
                    int scale, focus::TextAlignment alignment,
                    focus::GlyphProvider* provider) {
        this->clear(0);
        this->textSize = scale;
        this->textColor = 1;
        this->textLayoutRect = focus::MakeRect(0, 0, this->getCanvasWidth(),
                                               this->getCanvasHeight());
        this->cursor = this->textLayoutRect.origin;
        this->textAlignment = alignment;
        this->emphasisDepth = emphasisAtStart;
        this->lastWasNewline = false;
        this->lineSpacing = focus::TextLayout::calculateLineSpacing(provider);
        this->paragraphSpacing = focus::TextLayout::calculateParagraphSpacing(provider);
        this->glyphRowCount = provider->getGlyphRowCount();

        // Reserve the synthesized hyphen's width so alignment can't push it out.
        int16_t hyphenReserve = 0;
        if (trailingHyphen) {
            hyphenReserve = provider->metricsForCodepoint(
                '-', static_cast<focus::FontStyle>(this->emphasisDepth)).advance * scale;
        }
        this->renderBidiLine(codepoints, 0, len, 1,
                             (int16_t)(this->textLayoutRect.size.width - hyphenReserve),
                             0, provider);
        if (trailingHyphen) {
            this->writeCodepoint('-', provider);
        }
    }
};

}  // namespace

namespace focus {

TextView::TextView(Rect rect, std::string text) : View(rect) {
    this->text = text;
}

GlyphProvider* TextView::resolveGlyphProvider() const {
    if (this->font) return this->font->getGlyphProvider();
    std::shared_ptr<Font> sys = Font::systemFont();
    return sys ? sys->getGlyphProvider() : nullptr;
}

int TextView::heightForWidth(int width) const {
    GlyphProvider* provider = this->resolveGlyphProvider();
    if (!provider) return 0;
    return TextLayout::measureTextHeight(this->text.c_str(), (int16_t)width,
                                         this->textScale, provider);
}

void TextView::invalidateIndex() {
    this->indexValid = false;
    if (std::shared_ptr<Window> window = this->getWindow().lock()) {
        this->setNeedsDisplayInRect(this->frame);
    }
}

void TextView::setText(std::string text) {
    this->text = text;
    this->invalidateIndex();
}

void TextView::setTextScale(uint8_t scale) {
    this->textScale = scale;
    this->scratch = nullptr;
    this->invalidateIndex();
}

void TextView::setFont(std::shared_ptr<Font> font) {
    this->font = font;
    this->scratch = nullptr;
    this->invalidateIndex();
}

void TextView::setTextAlignment(TextAlignment alignment) {
    this->textAlignment = alignment;
    this->invalidateIndex();
}

void TextView::setFrame(Rect rect) {
    if (rect.size.width != this->frame.size.width) {
        this->indexValid = false;
        this->scratch = nullptr;
    }
    View::setFrame(rect);
}

void TextView::rebuildIndex() {
    this->lines.clear();
    this->maxLineCodepoints = 0;
    this->indexValid = true;

    GlyphProvider* provider = this->resolveGlyphProvider();
    if (!provider || this->text.empty() || this->frame.size.width <= 0) return;

    size_t len = utf8_codepoint_length(this->text.c_str());
    if (len == 0) return;
    UNICODE_CODEPOINT* codepoints =
        (UNICODE_CODEPOINT*)malloc(len * sizeof(UNICODE_CODEPOINT));
    if (!codepoints) return;
    utf8_parse(this->text.c_str(), codepoints);

    // Mirrors TextLayout::measureTextHeight so recorded positions and
    // heightForWidth always agree.
    int16_t lineSpacing = TextLayout::calculateLineSpacing(provider);
    int16_t paragraphSpacing = TextLayout::calculateParagraphSpacing(provider);
    int16_t lineHeight = TextLayout::getLineHeight(provider, this->textScale, lineSpacing);
    int16_t paragraphHeight = TextLayout::getParagraphHeight(provider, this->textScale, paragraphSpacing);

    int16_t y = 0;
    size_t offset = 0;
    uint32_t byteOffset = 0;
    uint8_t emphasis = 0;

    while (offset < len) {
        WordWrapResult result = TextLayout::measureLineWrap(
            codepoints + offset, len - offset,
            (int16_t)this->frame.size.width, this->textScale, provider,
            0, static_cast<FontStyle>(emphasis));

        size_t consumed = (result.codepointsConsumed < 0)
            ? (len - offset) : (size_t)result.codepointsConsumed;
        if (consumed == 0) break;  // defensive: a zero-advance line would loop forever

        LineRecord record;
        record.startByte = byteOffset;
        record.y = y;
        record.emphasisAtStart = emphasis;
        record.hasTrailingHyphen = result.needsHyphen;

        for (size_t i = 0; i < consumed; i++) {
            UNICODE_CODEPOINT cp = codepoints[offset + i];
            applyEmphasisShift(cp, emphasis);
            byteOffset += (uint32_t)TextLayout::bytesForCodepoint(cp);
        }
        record.endByte = byteOffset;
        this->lines.push_back(record);
        if (consumed > this->maxLineCodepoints) this->maxLineCodepoints = consumed;

        if (result.codepointsConsumed < 0) break;  // last line: no trailing spacing
        y += result.isParagraphBreak ? paragraphHeight : lineHeight;
        offset += consumed;
    }

    free(codepoints);
}

void TextView::drawContent(int x, int y, Rect clipRect) {
    if (!this->indexValid) this->rebuildIndex();
    if (this->lines.empty()) return;

    GlyphProvider* provider = this->resolveGlyphProvider();
    std::shared_ptr<Display> display = this->getDisplayIfAttached();
    if (!provider || !display) return;

    int rowHeight = provider->getGlyphRowCount() * this->textScale;
    if (rowHeight <= 0) return;

    if (!this->scratch) {
        this->scratch = std::make_shared<LineScratch>(
            MakeRect(0, 0, this->frame.size.width, rowHeight));
        if (this->font) this->scratch->setFont(this->font);
    }
    LineScratch* scratch = static_cast<LineScratch*>(this->scratch.get());

    int contentLeft = x + this->frame.origin.x;
    int contentTop = y + this->frame.origin.y;

    // Visible band in content coordinates; a zero-size clip means everything.
    int bandTop = 0, bandBottom = this->frame.size.height;
    if (clipRect.size.width > 0 && clipRect.size.height > 0) {
        bandTop = clipRect.origin.y - contentTop;
        bandBottom = clipRect.origin.y + clipRect.size.height - contentTop;
    }

    std::vector<UNICODE_CODEPOINT> lineCodepoints(this->maxLineCodepoints);
    for (const LineRecord& record : this->lines) {
        if (record.y + rowHeight <= bandTop) continue;
        if (record.y >= bandBottom) break;

        // Decode just this line's bytes.
        const char* cursor = this->text.data() + record.startByte;
        const char* end = this->text.data() + record.endByte;
        size_t count = 0;
        while (cursor < end) {
            UNICODE_CODEPOINT cp = utf8_next(cursor, end);
            if (cp == UTF8_END || cp == UTF8_ERROR) break;
            if (count == this->maxLineCodepoints) break;  // defensive: BMP-only byte drift
            lineCodepoints[count++] = cp;
        }

        scratch->renderLine(lineCodepoints.data(), count,
                            record.emphasisAtStart, record.hasTrailingHyphen,
                            this->textScale, this->textAlignment, provider);
        display->blitMasked(contentLeft, contentTop + record.y,
                            this->frame.size.width, rowHeight,
                            this->foregroundColor,
                            scratch->getBufferData(), scratch->getRowBytes(), clipRect);
    }
}

std::string TextView::accessibilityLabel() const {
    return this->text;
}

AccessibilityRole TextView::accessibilityRole() const {
    return AccessibilityRole::StaticText;
}

bool TextView::isAccessibilityElement() const {
    return true;
}

}  // namespace focus
