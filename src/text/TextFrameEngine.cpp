/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
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

#include "TextFrameEngine.hpp"
#include "TextLayout.hpp"
#include "ArabicShaping.hpp"
#include "utf8_parse.hpp"
#include <algorithm>

/// Track SO/SI emphasis changes through a range of codepoints.
static void scanEmphasis(const UNICODE_CODEPOINT* codepoints, size_t from, size_t to, uint8_t& depth) {
    for (size_t i = from; i < to; i++) {
        if (codepoints[i] == 0x0E) depth = std::min((uint8_t)(depth + 1), (uint8_t)3);
        else if (codepoints[i] == 0x0F) depth = depth > 0 ? depth - 1 : 0;
    }
}

FrameResult TextFrameEngine::layoutFrame(
    const char* utf8Text,
    size_t textLength,
    uint32_t fileByteOffset,
    const std::vector<StyleRun>& styleRuns,
    GlyphProvider* glyphProvider,
    const FrameConfig& config,
    FrameContinuation& state,
    bool isLastChunk
) {
    FrameResult result;
    result.complete = true;
    result.visibleByteEnd = fileByteOffset;

    if (textLength == 0 || glyphProvider == nullptr) {
        result.visibleByteEnd = fileByteOffset + textLength;
        return result;
    }

    // Parse UTF-8 to codepoints
    size_t codepointLen = utf8_codepoint_length((char*)utf8Text);
    if (codepointLen == 0) return result;

    std::vector<UNICODE_CODEPOINT> codepoints(codepointLen);
    utf8_parse((char*)utf8Text, codepoints.data());

    // Record byte lengths before shaping
    std::vector<uint8_t> byteLengths(codepointLen);
    for (size_t i = 0; i < codepointLen; i++) {
        byteLengths[i] = TextLayout::bytesForCodepoint(codepoints[i]);
    }

    // Arabic shaping
    bool needsShaping = false;
    for (size_t i = 0; i < codepointLen; i++) {
        if (codepoints[i] >= 0x0621 && codepoints[i] <= 0x06D2) {
            needsShaping = true;
            break;
        }
    }
    if (needsShaping) {
        shapeArabic(codepoints.data(), codepointLen);
    }

    size_t pos = 0;
    size_t totalBytesConsumed = 0;
    size_t styleRunIndex = 0;
    bool titleMode = false;

    // Compute indent unit from space width
    Rect spaceMetrics = glyphProvider->metricsForCodepoint(' ');
    int16_t indentPerLevel = spaceMetrics.size.width * config.textSize * 3;

    // If continuing a partial line from a previous chunk
    if (state.cursorX == 0) {
        state.lineStartByteOffset = fileByteOffset;
    }

    // Restore indent pixels from indent level (for continuation across chunks/pages)
    if (state.indentLevel > 0 && state.currentIndentPixels == 0) {
        state.currentIndentPixels = state.indentLevel * indentPerLevel;
    }

    // Advance style run index to the current byte offset
    while (styleRunIndex < styleRuns.size() &&
           styleRuns[styleRunIndex].byteOffset < fileByteOffset + totalBytesConsumed) {
        styleRunIndex++;
    }

    while (pos < codepointLen) {
        uint32_t currentByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;

        // --- Process style runs at the current byte position ---
        uint16_t bytesToSkip = 0;
        bool hadPageBreak = false;
        bool hadSceneBreak = false;

        while (styleRunIndex < styleRuns.size() &&
               styleRuns[styleRunIndex].byteOffset == currentByteOffset) {
            const StyleRun& run = styleRuns[styleRunIndex];

            switch (run.style) {
                case TextStyle::IndentLevel:
                    state.indentLevel = run.value;
                    state.currentIndentPixels = run.value * indentPerLevel;
                    state.atLineStart = false;
                    bytesToSkip = std::max(bytesToSkip, run.consumeBytes);
                    break;

                case TextStyle::PageBreakBefore:
                    hadPageBreak = true;
                    bytesToSkip = std::max(bytesToSkip, run.consumeBytes);
                    break;

                case TextStyle::SceneBreak:
                    hadSceneBreak = true;
                    bytesToSkip = std::max(bytesToSkip, run.consumeBytes);
                    break;

                case TextStyle::TitleMode:
                    titleMode = true;
                    break;
            }
            styleRunIndex++;
        }

        // Handle forced page break (from FF or FS/GS/RS)
        if (hadPageBreak) {
            scanEmphasis(codepoints.data(), pos, pos + bytesToSkip, state.emphasisDepth);
            if (state.cursorY > 0) {
                result.complete = false;
                result.visibleByteEnd = state.lineStartByteOffset;
                result.pageBreakEmphasisDepth = state.emphasisDepth;
                result.pageBreakIndentLevel = state.indentLevel;
                // Don't update state.cursorY — caller will see it's non-zero
                // and know content was present before the break
            }
            // Skip past the separator byte(s)
            size_t codepointsToSkip = 0;
            size_t skippedBytes = 0;
            while (codepointsToSkip < codepointLen - pos && skippedBytes < bytesToSkip) {
                skippedBytes += byteLengths[pos + codepointsToSkip];
                codepointsToSkip++;
            }
            pos += codepointsToSkip;
            totalBytesConsumed += skippedBytes;
            state.cursorY = 0;
            state.cursorX = 0;
            state.lastWasNewline = false;
            state.atLineStart = true;
            state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
            if (!result.complete) {
                result.bytesConsumed = totalBytesConsumed;
                return result;
            }
            continue;
        }

        // Handle scene break (from US)
        if (hadSceneBreak) {
            scanEmphasis(codepoints.data(), pos, pos + bytesToSkip, state.emphasisDepth);
            int16_t sceneBreakHeight = TextLayout::getParagraphHeight(
                glyphProvider, config.textSize, config.paragraphSpacing) * 2;

            // Skip past the separator byte(s)
            size_t codepointsToSkip = 0;
            size_t skippedBytes = 0;
            while (codepointsToSkip < codepointLen - pos && skippedBytes < bytesToSkip) {
                skippedBytes += byteLengths[pos + codepointsToSkip];
                codepointsToSkip++;
            }
            pos += codepointsToSkip;
            totalBytesConsumed += skippedBytes;
            state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
            state.lastWasNewline = false;
            state.atLineStart = true;
            state.cursorY += sceneBreakHeight;

            if (state.cursorY > config.layoutRect.size.height) {
                result.complete = false;
                result.visibleByteEnd = state.lineStartByteOffset;
                result.pageBreakEmphasisDepth = state.emphasisDepth;
                result.pageBreakIndentLevel = state.indentLevel;
                result.bytesConsumed = totalBytesConsumed;
                state.cursorY = 0;
                return result;
            }
            continue;
        }

        // Skip consumed bytes from style runs (IndentLevel DLE+'>' pairs)
        if (bytesToSkip > 0) {
            size_t codepointsToSkip = 0;
            size_t skippedBytes = 0;
            while (codepointsToSkip < codepointLen - pos && skippedBytes < bytesToSkip) {
                skippedBytes += byteLengths[pos + codepointsToSkip];
                codepointsToSkip++;
            }
            pos += codepointsToSkip;
            totalBytesConsumed += skippedBytes;
            state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
        }

        // At line start with no IndentLevel style run — reset indent
        if (state.atLineStart) {
            // If no IndentLevel was found at this position, indent level stays
            // from the previous line (for word-wrap continuation).
            // But if this is a true line start (after paragraph break),
            // and there was no IndentLevel style run, reset to 0.
            // Note: IndentLevel style runs handle their own atLineStart=false above.
            state.currentIndentPixels = 0;
            state.indentLevel = 0;
            state.atLineStart = false;
        }

        if (pos >= codepointLen) break;

        // --- Measure the next line ---
        int16_t effectiveWidth = config.layoutRect.size.width - 2 * state.currentIndentPixels;

        WordWrapResult wrapResult = TextLayout::measureLineWrap(
            codepoints.data() + pos,
            codepointLen - pos,
            effectiveWidth,
            config.textSize,
            glyphProvider,
            state.cursorX
        );

        if (wrapResult.codepointsConsumed < 0) {
            // No wrap needed — consumed rest of input (partial line)
            state.cursorX = wrapResult.endCursorX;

            size_t remainingBytes = 0;
            for (size_t i = pos; i < codepointLen; i++) {
                remainingBytes += byteLengths[i];
            }

            if (isLastChunk && remainingBytes > 0) {
                int16_t lineHeight = TextLayout::getLineHeight(
                    glyphProvider, config.textSize, config.lineSpacing);

                if (state.cursorY + lineHeight > config.layoutRect.size.height) {
                    result.complete = false;
                    result.visibleByteEnd = state.lineStartByteOffset;
                    state.cursorY = 0;
                    scanEmphasis(codepoints.data(), pos, codepointLen, state.emphasisDepth);
                    totalBytesConsumed += remainingBytes;
                    result.bytesConsumed = totalBytesConsumed;
                    return result;
                }

                result.lines.push_back({
                    state.lineStartByteOffset,
                    fileByteOffset + (uint32_t)(totalBytesConsumed + remainingBytes),
                    state.cursorY,
                    state.currentIndentPixels,
                    lineHeight,
                    state.emphasisDepth,
                    titleMode
                });

                state.cursorY += lineHeight;
                state.cursorX = 0;
                state.lineStartByteOffset = fileByteOffset + (uint32_t)(totalBytesConsumed + remainingBytes);
            }

            scanEmphasis(codepoints.data(), pos, codepointLen, state.emphasisDepth);
            totalBytesConsumed += remainingBytes;
            break;
        }

        // --- Line was completed (wrapped or paragraph break) ---
        state.cursorX = wrapResult.endCursorX; // Should be 0

        size_t consumedBytes = 0;
        for (size_t i = pos; i < pos + wrapResult.codepointsConsumed; i++) {
            consumedBytes += byteLengths[i];
        }

        // Check if measureLineWrap stopped at a separator that has a style run.
        // If so, the style run will be processed at the top of the next iteration.
        // For now, treat this as a normal paragraph break and let the style run
        // handler deal with it.
        if (wrapResult.isParagraphBreak && wrapResult.codepointsConsumed > 0) {
            UNICODE_CODEPOINT lastCp = codepoints[pos + wrapResult.codepointsConsumed - 1];
            // If the last codepoint is a separator with a style run, consume it
            // and let the style run handler at the top of the loop deal with it.
            if (lastCp == 0x0C || (lastCp >= 0x1C && lastCp <= 0x1F)) {
                // The separator is the last codepoint consumed by measureLineWrap.
                // We need to un-consume it so the style run handler processes it.
                // Actually, the style run is at the separator's byte offset, which
                // is within the consumed range. We need to handle this differently.
                //
                // Strategy: consume everything EXCEPT the separator. The next
                // iteration will hit the separator and find its style run.
                size_t preSepBytes = 0;
                for (size_t i = pos; i < pos + wrapResult.codepointsConsumed - 1; i++) {
                    preSepBytes += byteLengths[i];
                }

                // If there's content before the separator, emit it as a line
                if (wrapResult.codepointsConsumed > 1) {
                    int16_t lineHeight = TextLayout::getLineHeight(
                        glyphProvider, config.textSize, config.lineSpacing);

                    if (state.cursorY + lineHeight > config.layoutRect.size.height) {
                        result.complete = false;
                        result.visibleByteEnd = state.lineStartByteOffset;
                        result.bytesConsumed = totalBytesConsumed;
                        scanEmphasis(codepoints.data(), pos, pos + wrapResult.codepointsConsumed - 1, state.emphasisDepth);
                        state.cursorY = 0;
                        return result;
                    }

                    result.lines.push_back({
                        state.lineStartByteOffset,
                        fileByteOffset + (uint32_t)(totalBytesConsumed + preSepBytes),
                        state.cursorY,
                        state.currentIndentPixels,
                        lineHeight,
                        state.emphasisDepth,
                        titleMode
                    });

                    scanEmphasis(codepoints.data(), pos, pos + wrapResult.codepointsConsumed - 1, state.emphasisDepth);
                    state.cursorY += lineHeight;
                    if (titleMode) titleMode = false;
                }

                // Consume everything except the separator
                pos += wrapResult.codepointsConsumed - 1;
                totalBytesConsumed += preSepBytes;
                state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
                state.lastWasNewline = false;
                state.atLineStart = true;
                continue;
            }
        }

        // --- Normal line (word-wrap or newline paragraph break) ---

        // Calculate line height
        int16_t lineHeight = 0;
        bool isEmptyLine = (wrapResult.isParagraphBreak &&
                            codepoints[pos] == '\n' &&
                            state.lastWasNewline);
        if (isEmptyLine) {
            lineHeight = config.paragraphSpacing;
        } else if (wrapResult.isParagraphBreak || wrapResult.wrapped) {
            lineHeight = TextLayout::getLineHeight(
                glyphProvider, config.textSize, config.lineSpacing);
        }

        // Update newline and line-start tracking
        if (wrapResult.isParagraphBreak) {
            state.lastWasNewline = true;
            state.atLineStart = true;
        } else {
            state.lastWasNewline = false;
            // Word-wrap continuation: atLineStart stays false
        }

        // Check page overflow BEFORE consuming the line
        if (state.cursorY + lineHeight > config.layoutRect.size.height) {
            if (isEmptyLine) {
                // Drop empty lines at page boundaries
                scanEmphasis(codepoints.data(), pos, pos + wrapResult.codepointsConsumed, state.emphasisDepth);
                pos += wrapResult.codepointsConsumed;
                totalBytesConsumed += consumedBytes;
                state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
            }
            result.complete = false;
            result.visibleByteEnd = state.lineStartByteOffset;

            // Capture emphasis/indent at the page break boundary BEFORE consuming
            // the overflowing line. The page record needs this pre-overflow state
            // so the renderer can initialize correctly (the overflowing line is
            // re-laid-out when rendering the next page).
            result.pageBreakEmphasisDepth = state.emphasisDepth;
            result.pageBreakIndentLevel = state.indentLevel;

            if (!isEmptyLine) {
                // Consume the overflowing line on the new page (place it at
                // cursorY=0). This is critical for cross-chunk pagination:
                // when a partial line spans a chunk boundary, the line must be
                // consumed here so the caller advances past it correctly.
                scanEmphasis(codepoints.data(), pos, pos + wrapResult.codepointsConsumed, state.emphasisDepth);
                if (titleMode && wrapResult.isParagraphBreak) titleMode = false;
                pos += wrapResult.codepointsConsumed;
                totalBytesConsumed += consumedBytes;
                state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
                state.cursorY = lineHeight;
            } else {
                state.cursorY = 0;
            }

            result.bytesConsumed = totalBytesConsumed;
            return result;
        }

        // Record emphasis at the start of this line, then advance through it
        uint8_t emphasisAtLineStart = state.emphasisDepth;
        scanEmphasis(codepoints.data(), pos, pos + wrapResult.codepointsConsumed, state.emphasisDepth);

        // Emit the line (skip empty lines — they're just paragraph spacing)
        if (!isEmptyLine && lineHeight > 0) {
            result.lines.push_back({
                state.lineStartByteOffset,
                fileByteOffset + (uint32_t)(totalBytesConsumed + consumedBytes),
                state.cursorY,
                state.currentIndentPixels,
                lineHeight,
                emphasisAtLineStart,
                titleMode
            });
        }

        // Clear title mode after a paragraph break (title is one line only)
        if (titleMode && wrapResult.isParagraphBreak) {
            titleMode = false;
        }

        // Consume the line
        pos += wrapResult.codepointsConsumed;
        totalBytesConsumed += consumedBytes;
        state.lineStartByteOffset = fileByteOffset + (uint32_t)totalBytesConsumed;
        state.cursorY += lineHeight;
    }

    result.visibleByteEnd = fileByteOffset + (uint32_t)totalBytesConsumed;
    result.bytesConsumed = totalBytesConsumed;
    return result;
}
