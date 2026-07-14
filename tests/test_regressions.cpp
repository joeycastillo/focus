/*
 * Regression tests from known bugs.
 *
 * Each test reproduces a specific bug that was found and fixed. The test
 * encodes the exact input that triggered the original failure.
 */

#include "test_harness.hpp"
#include "MockGlyphProvider.hpp"
#include "RecordingDisplay.hpp"
#include "TextLayout.hpp"
#include "ArabicShaping.hpp"
#include "View.hpp"
#include "Application.hpp"
#include "Window.hpp"
#include "LabelView.hpp"
#include "Button.hpp"
#include "ProgressView.hpp"
#include "Display.hpp"
#include "Utf8.hpp"
#include <cstdlib>
#include <cstring>

using namespace focus;

// Helper: parse UTF-8 string into codepoint array. Caller must free().
static UNICODE_CODEPOINT* toCodepoints(const char* utf8, size_t* outLen) {
    size_t len = utf8_codepoint_length(utf8);
    UNICODE_CODEPOINT* cps = (UNICODE_CODEPOINT*)malloc(len * sizeof(UNICODE_CODEPOINT));
    utf8_parse(utf8, cps);
    *outLen = len;
    return cps;
}

// --- Minimal Application/Window for handleEvent tests ---

class RegressionTestWindow : public Window {
public:
    RegressionTestWindow(Size size) : Window(nullptr, size) {}
    void setApp(std::shared_ptr<Application> app) {
        this->application = app;
    }
};

class RegressionTestApp : public Application {
public:
    RegressionTestApp(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}
};

struct RegressionTestEnv {
    std::shared_ptr<RegressionTestWindow> window;
    std::shared_ptr<RegressionTestApp> app;
};

static RegressionTestEnv makeTestEnv() {
    auto window = std::make_shared<RegressionTestWindow>(MakeSize(480, 800));
    auto app = std::make_shared<RegressionTestApp>(window);
    window->setApp(app);
    return {window, app};
}

// --- measureLineWrap off-by-one (eb58ce1) ---
// Bug: text whose pixel width exactly equals layout width was being wrapped
// because the loop condition was `cursorX < layoutWidth` instead of
// `cursorX <= layoutWidth`.

TEST(regression_exact_width_no_wrap) {
    // "ABCDEFGHIJ" = 10 chars × 8px = 80px, layoutWidth = 80.
    // Must NOT wrap — cursor reaches 80 but that's exactly the boundary.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ABCDEFGHIJ", &len);
    ASSERT_EQ(len, (size_t)10);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    ASSERT_EQ(r.codepointsConsumed, (int32_t)-1);  // no wrap
    ASSERT_FALSE(r.wrapped);
    ASSERT_EQ(r.endCursorX, (int16_t)80);

    free(cps);
}

TEST(regression_exact_width_plus_one_wraps) {
    // "ABCDEFGHIJK" = 11 chars × 8px = 88px > 80. Must wrap.
    MockGlyphProvider gp(8);
    size_t len;
    UNICODE_CODEPOINT* cps = toCodepoints("ABCDEFGHIJK", &len);
    ASSERT_EQ(len, (size_t)11);

    WordWrapResult r = TextLayout::measureLineWrap(cps, len, 80, 1, &gp);
    ASSERT_TRUE(r.wrapped);

    free(cps);
}

// --- BiDi numbers break shaping (tranquil-leaping-seahorse) ---
// Bug: Arabic text with embedded numbers was losing contextual shaping
// because the number characters broke the connectivity chain.

TEST(regression_arabic_numbers_break_shaping) {
    // Arabic Baa (U+0628) + number "5" + Arabic Baa (U+0628)
    // The number should break the connection: both Baas should be isolated.
    UNICODE_CODEPOINT input[] = {0x0628, '5', 0x0628};
    std::vector<UNICODE_CODEPOINT> buf(input, input + 3);

    shapeArabic(buf.data(), buf.size());

    // Both Baas should be in isolated form since the number breaks the chain.
    // Isolated Baa = U+FE8F
    ASSERT_EQ(buf[0], (UNICODE_CODEPOINT)0xFE8F);
    ASSERT_EQ(buf[2], (UNICODE_CODEPOINT)0xFE8F);
}

// --- Event consumption: SELECT with both SELECT and TOUCH_UP_INSIDE ---
// Bug: when both SELECT and TOUCH_UP_INSIDE actions were registered,
// SELECT events should match the SELECT action directly without
// falling through to the TOUCH_UP_INSIDE fallback.

TEST(regression_select_prefers_direct_match_over_fallback) {
    auto env = makeTestEnv();
    auto view = std::make_shared<View>(MakeRect(0, 0, 100, 100));
    env.window->addSubview(view);

    bool selectCalled = false;
    bool touchUpCalled = false;

    view->setAction([&](Event e, std::weak_ptr<View> sender) {
        selectCalled = true;
    }, FOCUS_EVENT_SELECT);

    view->setAction([&](Event e, std::weak_ptr<View> sender) {
        touchUpCalled = true;
    }, FOCUS_EVENT_TOUCH_UP_INSIDE);

    Event event = {FOCUS_EVENT_SELECT, 0, 0};
    view->handleEvent(event);

    ASSERT_TRUE(selectCalled);
    ASSERT_FALSE(touchUpCalled);
}

// --- Display::flush() dispatch ---
// Display::flush() dispatches through the base pointer; the default is a no-op.

struct FlushCountingDisplay : public focus::Display {
    int flushCount = 0;
    void fillRect(int, int, int, int, uint16_t, focus::Rect = {{0,0},{0,0}}) override {}
    void blitMasked(int, int, int, int, uint16_t, const uint8_t*, int,
                    focus::Rect = {{0,0},{0,0}}) override {}
    void flush(focus::Rect) override { flushCount++; }
};

TEST(display_flush_dispatches_via_base) {
    FlushCountingDisplay concrete;
    focus::Display* base = &concrete;
    base->flush({{0, 0}, {10, 10}});
    ASSERT_EQ(concrete.flushCount, 1);

    // Default implementation is callable and harmless.
    struct MinimalDisplay : public focus::Display {
        void fillRect(int, int, int, int, uint16_t, focus::Rect = {{0,0},{0,0}}) override {}
        void blitMasked(int, int, int, int, uint16_t, const uint8_t*, int,
                        focus::Rect = {{0,0},{0,0}}) override {}
    } minimal;
    static_cast<focus::Display*>(&minimal)->flush({{0, 0}, {1, 1}});
}

// --- No-op value setters ---
// Writing the value a view already displays must not dirty the window;
// on unbuffered displays every needless repaint reaches the panel.

TEST(value_setters_skip_invalidation_when_unchanged) {
    auto display = std::make_shared<RecordingDisplay>(160, 128);
    auto window = std::make_shared<Window>(display, MakeSize(160, 128));

    auto label = std::make_shared<LabelView>(MakeRect(0, 0, 100, 10), "hello");
    auto button = std::make_shared<Button>(MakeRect(0, 20, 100, 20), "Play");
    auto progress = std::make_shared<ProgressView>(MakeRect(0, 50, 100, 8));
    window->addSubview(label);
    window->addSubview(button);
    window->addSubview(progress);
    progress->setProgress(0.5f);
    window->clearNeedsDisplay();

    label->setText("hello");
    button->setTitle("Play");
    progress->setProgress(0.5f);
    ASSERT_FALSE(window->needsDisplay());

    label->setText("world");
    ASSERT_TRUE(window->needsDisplay());
    window->clearNeedsDisplay();

    button->setTitle("Pause");
    ASSERT_TRUE(window->needsDisplay());
    window->clearNeedsDisplay();

    button->setTitle("Held", ControlState::Selected);
    ASSERT_TRUE(window->needsDisplay());
    window->clearNeedsDisplay();
    button->setTitle("Held", ControlState::Selected);
    ASSERT_FALSE(window->needsDisplay());

    progress->setProgress(1.0f);
    ASSERT_TRUE(window->needsDisplay());
    window->clearNeedsDisplay();

    // Clamped writes compare post-clamp: 1.5 clamps to the current 1.0.
    progress->setProgress(1.5f);
    ASSERT_FALSE(window->needsDisplay());
}
