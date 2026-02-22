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

#include "EdgeDragGestureRecognizer.hpp"
#include <cstdlib>

EdgeDragGestureRecognizer::EdgeDragGestureRecognizer(Rect activationRegion, int moveThreshold)
    : activationRegion(activationRegion), moveThreshold(moveThreshold) {
}

bool EdgeDragGestureRecognizer::wantsTouch(Point windowPoint) {
    return windowPoint.x >= this->activationRegion.origin.x &&
           windowPoint.x < this->activationRegion.origin.x + this->activationRegion.size.width &&
           windowPoint.y >= this->activationRegion.origin.y &&
           windowPoint.y < this->activationRegion.origin.y + this->activationRegion.size.height;
}

void EdgeDragGestureRecognizer::touchDown(Event event) {
    this->touchDownPoint = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
    this->state = State::Possible;
}

void EdgeDragGestureRecognizer::touchMoved(Event event) {
    if (this->state == State::Failed) return;

    Point current = MakePoint(event.userInfo >> 16, event.userInfo & 0xFFFF);
    int dx = current.x - this->touchDownPoint.x;
    int dy = current.y - this->touchDownPoint.y;
    int dist = abs(dx) + abs(dy);

    if (this->state == State::Possible) {
        if (dist > this->moveThreshold) {
            this->state = State::Recognized;
            if (this->onRecognized) this->onRecognized(event);
            if (this->onMoved) this->onMoved(event);
        }
    } else if (this->state == State::Recognized) {
        if (this->onMoved) this->onMoved(event);
    }
}

void EdgeDragGestureRecognizer::touchUp(Event event) {
    if (this->state == State::Possible) {
        // Touch ended before threshold — was a tap, not a drag
        this->state = State::Failed;
    } else if (this->state == State::Recognized) {
        if (this->onEnded) this->onEnded(event);
    }
}

void EdgeDragGestureRecognizer::longPress(Event event) {
    if (this->state == State::Possible) {
        // Held still too long — not a drag gesture
        this->state = State::Failed;
    }
}

void EdgeDragGestureRecognizer::reset() {
    this->state = State::Possible;
    this->touchDownPoint = PointZero;
}
