#include "app/viewcontrollers/NowPlayingViewController.hpp"
#include "app/PlayerApp.hpp"
#include "app/PlayerNotifications.hpp"

#include "StackView.hpp"
#include "LabelView.hpp"
#include "ProgressView.hpp"
#include "Button.hpp"
#include "Timer.hpp"
#include "NotificationCenter.hpp"
#include "Locale.hpp"
#include "Focus.hpp"
#include <cstdio>

using namespace focus;

NowPlayingViewController::NowPlayingViewController(std::shared_ptr<PlayerApp> application) : ViewController(application) {
    // Title is shown in the navigation bar while this controller is atop the stack.
    // Has no effect for a view that's not in a navigation controller.
    this->setTitle(_LS("nowplaying.title", "Now Playing"));
}

// Build the view hierarchy here and assign it to this->view. Focus calls
// createView() each time this screen is about to appear without a view: the
// first time, and again after it was hidden. Build from state the app keeps,
// as updateNowPlaying() does at the end.
void NowPlayingViewController::createView() {
    this->accessibilityIdentifier = "now-playing";
    // Views are opaque by default. Keeping the root opaque lets Focus see
    // that it covers the views behind it and skip their background fills.
    this->view = std::make_shared<View>(RectZero);

    // VStack lays children out top-to-bottom.
    // Margins inset the edges; spacing adds space between items.
    auto stack = std::make_shared<VStack>(RectZero);
    stack->setMargins(2);
    stack->setSpacing(2);

    // In a VStack, width fills the full stack width, and a nonzero height is fixed. 
    // The height of 10 in this line creates a 10px-tall row.
    this->titleLabel = std::make_shared<LabelView>(MakeRect(0, 0, 0, 10), "");
    stack->addSubview(this->titleLabel);

    // 8 px tall for the song progress...
    this->progressView = std::make_shared<ProgressView>(MakeRect(0, 0, 0, 8));
    stack->addSubview(this->progressView);
    
    // 10 px tall for the time label...
    this->timeLabel = std::make_shared<LabelView>(MakeRect(0, 0, 0, 10), "--:-- / --:--");
    stack->addSubview(this->timeLabel);
    
    // ...and then a HStack for the controls.
    // HStack lays children left-to-right, perfect for our row of buttons.
    auto transport = std::make_shared<HStack>(MakeRect(0, 0, 0, 20));
    transport->setSpacing(2);

    auto previous = std::make_shared<Button>(MakeRect(0, 0, 0, 20), "<<");
    // Controls fire actions: a callback registered for an event type. TOUCH_UP_INSIDE
    // is a tap (and d-pad SELECT). When the event is triggered, the closure is called.
    // The trailing owner argument, weak_from_this(), lets Focus drop the action
    // automatically when this controller is destroyed.
    // IN THIS CASE, these buttons live in our own view, so they'd be torn down with us
    // regardless; passing an owner matters most for actions on views that outlive the
    // object that set up the action in the first place, like the Window or subviews
    // that you add to it. Still, passing this->weak_from_this() is a harmless habit
    // even where it's redundant, as it is here.
    previous->setAction([this](Event, std::weak_ptr<View>) {
        if (auto app = std::static_pointer_cast<PlayerApp>(this->application.lock())) {
            app->playPrevious();
        }
    }, FOCUS_EVENT_TOUCH_UP_INSIDE, this->weak_from_this());
    transport->addSubview(previous);

    // In a HStack, a nonzero width is fixed, but a zero width triggers flexible layout.
    // The width of 0 here (and in the previous button above and next button below) means
    // the button will share the available space with other views in the stack.
    this->playPauseButton = std::make_shared<Button>(MakeRect(0, 0, 0, 20), "||");
    this->playPauseButton->setAction([this](Event, std::weak_ptr<View>) {
        if (auto app = std::static_pointer_cast<PlayerApp>(this->application.lock())) {
            app->togglePlayPause();
            this->updateProgress();
        }
    }, FOCUS_EVENT_TOUCH_UP_INSIDE, this->weak_from_this());
    transport->addSubview(this->playPauseButton);

    auto next = std::make_shared<Button>(MakeRect(0, 0, 0, 20), ">>");
    next->setAction([this](Event, std::weak_ptr<View>) {
        if (auto app = std::static_pointer_cast<PlayerApp>(this->application.lock())) {
            app->playNext();
        }
    }, FOCUS_EVENT_TOUCH_UP_INSIDE, this->weak_from_this());
    transport->addSubview(next);

    stack->addSubview(transport);
    this->view->addSubview(stack);
    this->stack = stack;

    this->updateNowPlaying();
}

// Potential footgun: even when you give a view a size in createView — which, as a side
// note, we didn't do up above here — if your view ends up in a container like a
// NavigationViewController, THAT view controller can resize your view to the space it
// should occupy, and that happens AFTER createView(). Focus calls viewDidLayoutSubviews
// once that frame has landed, so size-dependent work should always go here.
void NowPlayingViewController::viewDidLayoutSubviews() {
    if (this->stack) {
        // In this case we just need to stretch the stack to fill the view.
        this->stack->setFrame(MakeRect(0, 0,
            this->view->getFrame().size.width,
            this->view->getFrame().size.height));
    }
}

// Now on screen. Start per-appearance work like observers and timers here,
// and tear it down in viewWillDisappear().
void NowPlayingViewController::viewDidAppear() {
    auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
    if (!app) return;

    // Observe track changes; passing this as the owner auto-removes the
    // observer when the controller is destroyed.
    this->trackChangedToken = NotificationCenter::shared()->addObserver(
        player::kTrackChanged,
        [this](const Notification&) { this->updateNowPlaying(); },
        this->shared_from_this());

    // Tick the progress bar and time label once a second.
    this->progressTimer = Timer::scheduledTimer(app, std::chrono::milliseconds(1000),
        [this](Timer&) { this->updateProgress(); }, true);
}

// Balance viewDidAppear(): stop the timer and drop the observer.
void NowPlayingViewController::viewWillDisappear() {
    if (this->progressTimer) this->progressTimer->invalidate();
    NotificationCenter::shared()->removeObserver(this->trackChangedToken);
}

// Pull the current track from the app model into the title label.
void NowPlayingViewController::updateNowPlaying() {
    auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
    if (!app || app->getLibrary().empty()) return;
    this->titleLabel->setText(app->getLibrary()[app->getCurrentIndex()].title);
    this->updateProgress();
}

// Refresh the progress bar, time label, and play/pause title from the engine.
void NowPlayingViewController::updateProgress() {
    auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
    if (!app || app->getLibrary().empty()) return;

    uint32_t elapsed = app->getEngine().elapsedMs();
    uint32_t duration = app->getLibrary()[app->getCurrentIndex()].durationMs;

    this->progressView->setProgress(duration ? (float)elapsed / (float)duration : 0.0f);
    std::string durationText = duration ? formatTime(duration) : "--:--";
    this->timeLabel->setText(formatTime(elapsed) + " / " + durationText);
    this->playPauseButton->setTitle(app->getEngine().isPlaying() ? "||" : "|>");
}

std::string NowPlayingViewController::formatTime(uint32_t ms) {
    uint32_t totalSeconds = ms / 1000;
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%u:%02u",
             (unsigned)(totalSeconds / 60), (unsigned)(totalSeconds % 60));
    return std::string(buffer);
}
