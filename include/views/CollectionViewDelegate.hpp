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
 * @file CollectionViewDelegate.hpp
 * @brief Delegate protocol for CollectionView selection events.
 *
 * Implement this interface to respond to item selection in a CollectionView.
 * When a delegate is set on a CollectionView, cells are automatically wired
 * to call didSelectItemAtIndex on touch-up-inside events.
 */

#pragma once

#include <cstddef>

class CollectionView;

/**
 * @brief Interface for handling CollectionView item selection.
 *
 * Methods have default empty implementations so that implementers only
 * need to override the callbacks they care about.
 */
class CollectionViewDelegate {
public:
    /**
     * @brief Called when the user selects an item in the collection view.
     * @param collectionView The collection view where the selection occurred.
     * @param index The index of the selected item (0-based, global).
     */
    virtual void didSelectItemAtIndex(CollectionView* collectionView, size_t index) {}

    virtual ~CollectionViewDelegate() {}
};
