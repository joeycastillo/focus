#include "app/PlayerApp.hpp"
#include "app/PlayerNotifications.hpp"
#include "app/RefreshTask.hpp"
#include "app/viewcontrollers/LibraryViewController.hpp"

#include "Window.hpp"
#include "Display.hpp"
#include "NotificationCenter.hpp"
#include "NavigationViewController.hpp"

using namespace focus;

PlayerApp::PlayerApp(const std::shared_ptr<Window>& window,
                     std::shared_ptr<Display> display,
                     std::shared_ptr<PlayerEngine> engine)
    : Application(window), display(display), engine(engine) {}

void PlayerApp::setup() {
    this->addTask(std::make_shared<RefreshTask>(this->display));
    this->library = this->engine->scanLibrary();

    NotificationCenter::shared()->addObserver(player::kPlaybackEnded,
        [this](const Notification&) { this->playNext(); });

    auto libraryVC = std::make_shared<LibraryViewController>(this->shared_from_this());
    this->setRootViewController(
        NavigationViewController::create(this->shared_from_this(), libraryVC));
}

PlayerEngine& PlayerApp::getEngine() { return *this->engine; }
const std::vector<TrackInfo>& PlayerApp::getLibrary() const { return this->library; }
size_t PlayerApp::getCurrentIndex() const { return this->currentIndex; }

void PlayerApp::rescanLibrary() {
    this->library = this->engine->scanLibrary();
}

void PlayerApp::playTrackAtIndex(size_t index) {
    if (this->library.empty()) return;
    // Try each track once, starting at index; stop after a full failed lap.
    for (size_t attempt = 0; attempt < this->library.size(); attempt++) {
        this->currentIndex = (index + attempt) % this->library.size();
        if (this->engine->play(this->library[this->currentIndex])) {
            NotificationCenter::shared()->post(player::kTrackChanged, this->currentIndex);
            return;
        }
    }
}

void PlayerApp::playNext() {
    this->playTrackAtIndex(this->currentIndex + 1);
}

void PlayerApp::playPrevious() {
    this->playTrackAtIndex(
        (this->currentIndex + this->library.size() - 1) % this->library.size());
}

void PlayerApp::togglePlayPause() {
    if (this->engine->isPlaying()) this->engine->pause();
    else this->engine->resume();
}
