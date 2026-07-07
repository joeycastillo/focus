#pragma once

namespace player {

/// Posted by PlayerApp when a track starts. userInfo: size_t library index.
inline const char* kTrackChanged = "player.trackChanged";

/// Posted by an engine task when the current track finishes.
inline const char* kPlaybackEnded = "player.playbackEnded";

}  // namespace player
