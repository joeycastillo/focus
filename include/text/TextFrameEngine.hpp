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
 * @par Two usage patterns
 * This engine serves two roles that must produce identical layout decisions:
 *
 * **Single-page rendering.** Given a page's byte range (from a database or
 * other page-break store), parse it into StyleRuns and call layoutFrame once
 * with `isLastChunk=true`. The resulting TextLines are passed to
 * CanvasView::drawStyledFrame for rendering. The caller initializes
 * FrameContinuation from the stored page-break state (emphasis, indent, etc.)
 * so rendering starts in the correct typographic context.
 *
 * **Chunked pagination.** Read the source file in fixed-size chunks and call
 * layoutFrame repeatedly with `isLastChunk=false` (except the final chunk).
 * Each call returns a FrameResult; when `complete` is false, the page
 * overflowed and `visibleByteEnd` marks where to split. The caller records
 * the page break and continues with the next page. FrameContinuation carries
 * all state between calls — the engine itself is stateless.
 *
 * @par Why the engine is stateless
 * FrameContinuation is a plain value type that the caller owns and manages.
 * The engine reads it at the start of layoutFrame and writes it back at the
 * end. This makes the engine a pure function of its inputs and avoids hidden
 * coupling between pagination and rendering — both callers provide the same
 * types, and the engine treats them identically.
 *
 * @par In-band control codes
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
///
/// When `complete` is true, all input text fit within the layout area.
/// When false, the frame overflowed and the fields below describe where to
/// split for pagination.
struct FrameResult {
    std::vector<TextLine> lines;    ///< Positioned lines in the frame.

    /// File byte offset where visible content ends — the page boundary.
    /// A paginator stores this as the start of the next page.
    uint32_t visibleByteEnd;

    /// Total bytes consumed from the input buffer. During overflow, the engine
    /// reads past visibleByteEnd to advance continuation state through the
    /// overflowing line (scanning emphasis codes, etc.). The caller should
    /// advance its file position by bytesConsumed, not visibleByteEnd.
    size_t bytesConsumed = 0;

    bool complete;                  ///< True if all input text was laid out without overflow.

    /// @name Page-break continuation state
    /// When the engine overflows, it advances continuation state past the
    /// overflowing line (since that line belongs to the next page). But the
    /// page-break record needs the state at the START of the overflowing
    /// line — because the renderer will re-lay-out from there.
    /// These fields capture that pre-overflow snapshot. Only meaningful
    /// when `complete` is false.
    /// @{
    uint8_t pageBreakEmphasisDepth = 0;
    uint8_t pageBreakIndentLevel = 0;
    bool pageBreakLastWasNewline = false;
    bool pageBreakAtLineStart = true;
    /// @}
};

/// Continuation state carried across frames (pages) or chunks.
///
/// This is a plain value type — the caller owns it and the engine updates it.
/// For single-page rendering, initialize from stored page-break state and
/// discard after the call. For chunked pagination, pass the same instance
/// across successive layoutFrame calls; the engine picks up where it left off.
///
/// All fields here are general-purpose text layout state — they describe
/// where the engine is in the text stream and what typographic context is
/// active. None are specific to any particular file format.
struct FrameContinuation {
    int16_t cursorY = 0;                ///< Vertical position (pixels from top of layout rect).
    int16_t cursorX = 0;                ///< Horizontal position within the current line.
    uint32_t lineStartByteOffset = 0;   ///< File byte offset where the current line started.
    bool lastWasNewline = false;         ///< Previous byte was a newline (controls paragraph spacing).
    bool atLineStart = true;            ///< At the start of a logical line (controls indent application).
    uint8_t emphasisDepth = 0;          ///< SO/SI emphasis depth: 0=normal, 1=italic, 2=bold, 3=bold+italic.
    uint8_t indentLevel = 0;            ///< Block quote nesting depth (set by IndentLevel style runs).
    int16_t currentIndentPixels = 0;    ///< Indent in pixels (derived from indentLevel and font metrics).
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
     * @param isLastChunk True if this is the final chunk of text. When false,
     *        the engine will not consume a partial line at the end of the buffer.
     *        Instead, those bytes remain unconsumed so the caller can prepend
     *        them to the next chunk, ensuring the full line is measured in a
     *        single call. This prevents word-wrap disagreements between the
     *        paginator (which processes text in chunks) and the renderer
     *        (which lays out each page in a single call).
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
