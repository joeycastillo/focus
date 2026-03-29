/*
 * MIT License
 *
 * Copyright (c) 2022-2025 Joey Castillo
 */

#include "CursorManager.hpp"

void CursorManager::setGain(float gain) {
    trackX.gain = gain;
    trackY.gain = gain;
}

void CursorManager::getPosition(int &px, int &py) const {
    px = (int)trackX.position;
    py = (int)trackY.position;
}

void CursorManager::setPosition(int px, int py) {
    trackX.position = (float)px;
    trackY.position = (float)py;
}

void CursorManager::handleTouchEvent(Event event, int maxX, int maxY) {
    int px = event.userInfo >> 16;
    int py = event.userInfo & 0xFFFF;

    if (event.type == FOCUS_EVENT_TOUCH_DOWN) {
        trackX.anchor(px);
        trackY.anchor(py);
        if (onTrackingBegan) onTrackingBegan();
    } else if (event.type == FOCUS_EVENT_TOUCH_MOVED) {
        trackX.track(px, 0, (float)maxX);
        trackY.track(py, 0, (float)maxY);
        if (onCursorMoved) onCursorMoved((int)trackX.position, (int)trackY.position);
    } else if (event.type == FOCUS_EVENT_TOUCH_UP) {
        if (onTrackingEnded) onTrackingEnded();
    }
}
