#pragma once

#include "CollectionViewController.hpp"

/// Track list. When the library is empty, shows a message cell naming the
/// expected music location plus a Rescan cell that remounts and re-reads.
class LibraryViewController : public focus::CollectionViewController {
public:
    LibraryViewController(std::shared_ptr<focus::Application> application);

    size_t numberOfItems() const override;
    std::shared_ptr<focus::CollectionViewCell> cellForItemAtIndex(
        size_t index, focus::Rect frame) override;
    void didSelectItemAtIndex(size_t index) override;

    void didFocusItemAtIndex(focus::CollectionView* collectionView, size_t index,
                             focus::CollectionViewCell& cell) override;
    void didUnfocusItemAtIndex(focus::CollectionView* collectionView, size_t index,
                               focus::CollectionViewCell& cell) override;

private:
    bool libraryIsEmpty() const;
};
