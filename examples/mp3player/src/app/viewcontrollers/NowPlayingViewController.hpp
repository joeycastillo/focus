#pragma once

#include "ViewController.hpp"
#include <memory>
#include <cstdint>

namespace focus {
class VStack;
class LabelView;
class ProgressView;
class Button;
class Timer;
}
class PlayerApp;

/// Transport screen: track title, progress, elapsed/duration, prev/play/next.
class NowPlayingViewController : public focus::ViewController {
public:
    NowPlayingViewController(std::shared_ptr<PlayerApp> application);

    void createView() override;
    void viewDidLayoutSubviews() override;
    void viewDidAppear() override;
    void viewWillDisappear() override;

private:
    void updateNowPlaying();
    void updateProgress();
    static std::string formatTime(uint32_t ms);

    std::shared_ptr<focus::VStack> stack;
    std::shared_ptr<focus::LabelView> titleLabel;
    std::shared_ptr<focus::ProgressView> progressView;
    std::shared_ptr<focus::LabelView> timeLabel;
    std::shared_ptr<focus::Button> playPauseButton;
    std::shared_ptr<focus::Timer> progressTimer;
    uint32_t trackChangedToken = 0;
};
