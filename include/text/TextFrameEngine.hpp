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

/**
 * @file TextFrameEngine.hpp
 * @brief Generic text layout engine for the Focus UI framework.
 *
 * TextFrameEngine takes UTF-8 text, an optional set of StyleRuns, and a
 * layout rectangle, and produces a FrameResult describing where each line
 * of text should be positioned. It handles word-wrapping, paragraph breaks,
 * and style-run-driven layout instructions (page breaks, scene breaks,
 * indentation, title mode).
 *
 * This engine is used by both the paginator (to find page boundaries) and
 * the renderer (to position lines for drawing). By sharing a single layout
 * implementation, pagination and rendering produce identical results.
 *
 * In-band control codes handled:
 *   SO/SI (0x0E/0x0F) — emphasis depth tracking (for continuation state)
 *   BS    (0x08)       — backspace width adjustment (handled by measureLineWrap)
 */

#pragma once

#include "Focus.hpp"
#include "StyledText.hpp"
#include "GlyphProvider.hpp"
#include <vector>
#include <cstdint>

/// A positioned line of text produced by the framing engine.
struct TextLine {
    uint32_t startByteOffset;       ///< File byte offset of first content byte on this line.
    uint32_t endByteOffset;         ///< File byte offset past last byte on this line.
    int16_t y;                      ///< Vertical position of the line (top edge).
    int16_t indent;                 ///< Horizontal indent in pixels (for block quotes).
    int16_t height;                 ///< Line height including spacing.
    uint8_t emphasisDepthAtStart;   ///< Emphasis state at the start of this line.
    bool isTitleLine;               ///< True if this line should render as bold title.
};

/// Result of laying out text into a frame.
struct FrameResult {
    std::vector<TextLine> lines;    ///< Positioned lines in the frame.
    uint32_t visibleByteEnd;        ///< File byte offset where visible content ends (page boundary).
    size_t bytesConsumed = 0;       ///< Bytes consumed from the input (may exceed visibleByteEnd at overflow).
    bool complete;                  ///< True if all input text was laid out without overflow.

    /// Emphasis and indent at the page break boundary.
    /// When the engine overflows, it consumes the overflowing line and advances
    /// continuation state past it. But the page record needs the state at the
    /// START of the overflowing line (since the renderer re-lays-out from there).
    /// These fields capture that pre-overflow state. Only meaningful when !complete.
    uint8_t pageBreakEmphasisDepth = 0;
    uint8_t pageBreakIndentLevel = 0;
};

/// Continuation state carried across frames (pages) or chunks.
struct FrameContinuation {
    int16_t cursorY = 0;                ///< Current vertical position.
    int16_t cursorX = 0;                ///< Current horizontal position (for partial lines across chunks).
    uint32_t lineStartByteOffset = 0;   ///< Byte offset where the current line started.
    bool lastWasNewline = false;         ///< True if previous line ended with a newline.
    bool atLineStart = true;             ///< True if we're at the start of a logical line.
    uint8_t emphasisDepth = 0;           ///< Current SO/SI emphasis depth (0–3).
    uint8_t indentLevel = 0;             ///< Current block quote nesting depth.
    int16_t currentIndentPixels = 0;     ///< Current indent in pixels.
};

/// Configuration for the framing engine.
struct FrameConfig {
    Rect layoutRect;                     ///< Available text area.
    uint8_t textSize = 1;               ///< Text scale factor.
    int16_t lineSpacing = 2;            ///< Extra pixels between lines.
    int16_t paragraphSpacing = 8;       ///< Extra pixels after paragraph breaks.
};

/// Text layout engine that produces positioned lines from text and style runs.
class TextFrameEngine {
public:
    /**
     * @brief Lay out text into a frame, producing positioned lines.
     *
     * @param utf8Text Raw UTF-8 text (control codes still in-band).
     * @param textLength Length in bytes.
     * @param fileByteOffset Byte offset of utf8Text[0] in the original file.
     * @param styleRuns Layout instructions sorted by byteOffset.
     * @param glyphProvider Font metrics source.
     * @param config Layout configuration.
     * @param state Continuation state (in/out — updated with end-of-frame state).
     * @param isLastChunk True if this is the final chunk of text.
     * @return FrameResult with positioned lines and continuation info.
     */
    static FrameResult layoutFrame(
        const char* utf8Text,
        size_t textLength,
        uint32_t fileByteOffset,
        const std::vector<StyleRun>& styleRuns,
        GlyphProvider* glyphProvider,
        const FrameConfig& config,
        FrameContinuation& state,
        bool isLastChunk = true
    );
};
