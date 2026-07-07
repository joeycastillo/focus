#!/bin/sh
# examples/mp3player/tests/run-hybrid-e2e.sh
# Runs the hybrid-input E2E script against a generated two-track library.
set -e
cd "$(dirname "$0")/../"
BUILD=build
MUSIC=$BUILD/e2e-music

cmake -S . -B $BUILD >/dev/null && cmake --build $BUILD -j >/dev/null

mkdir -p $MUSIC
if [ ! -f "$MUSIC/track-a.mp3" ] || [ ! -f "$MUSIC/track-b.mp3" ]; then
    ffmpeg -loglevel error -y -f lavfi -i "sine=frequency=440:duration=2" "$MUSIC/track-a.mp3"
    ffmpeg -loglevel error -y -f lavfi -i "sine=frequency=660:duration=2" "$MUSIC/track-b.mp3"
fi

LOG=/tmp/hybrid-e2e.log
$BUILD/mp3player --input=hybrid --script --music=$MUSIC < tests/hybrid.script 2>&1 | tee "$LOG"
! grep -q '^ERR' "$LOG"
