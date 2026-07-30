#include "app/viewcontrollers/LibraryViewController.hpp"
#include "app/viewcontrollers/NowPlayingViewController.hpp"
#include "app/PlayerApp.hpp"

#include "CollectionViewCell.hpp"
#include "LabelView.hpp"
#include "NavigationViewController.hpp"
#include "Focus.hpp"
#include "Color.hpp"
#include "Locale.hpp"

using namespace focus;

// Invert a row's colors to indicate focus.
static void setCellInverted(CollectionViewCell& cell, bool inverted) {
    uint16_t background = inverted ? GrayscaleColor::Black() : GrayscaleColor::White();
    uint16_t foreground = inverted ? GrayscaleColor::White() : GrayscaleColor::Black();
    cell.setBackgroundColor(background);
    for (const auto& subview : cell.getSubviews()) {
        subview->setForegroundColor(foreground);
        subview->setBackgroundColor(background);
    }
}

LibraryViewController::LibraryViewController(std::shared_ptr<Application> application)
    : CollectionViewController(application) {
    // So, yeah: this screen builds no view hierarchy of its own.
    // CollectionViewController is powerful that way: it creates the collection view as
    // our root view, and calls back into this class as both its data source (how many
    // items, what each contains) and its delegate (who's in focus, what's getting
    // selected). All we have to do is configure it here and answer some callbacks.

    // Configure layout up front; the base applies it when the view is built. A vertical
    // list of rows, each 24px tall and full-width (0 means "fill up this axis").
    this->setItemSize(MakeSize(0, 24));
    this->setLayout(CollectionViewLayout::VerticalList);

    // Set a title. This has no visual effect unless the view is embedded in a container
    // view controller like a NavigationViewController, but in this case, we are, so the
    // title appears up top.
    // Also note that we don't set a literal string, but instead use the _LS or
    // "localized string" call. The first parameter is the key to look up in the strings
    // file; the second serves as the fallback text if not found, as well as a comment
    // for any automated tooling aimed at string extraction.
    this->setTitle(_LS("library.title", "Music"));
    this->accessibilityIdentifier = "library";
}

bool LibraryViewController::libraryIsEmpty() const {
    auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
    return !app || app->getLibrary().empty();
}

// Data source, question 1: how many rows? The collection view asks this to lay itself out.
size_t LibraryViewController::numberOfItems() const {
    if (this->libraryIsEmpty()) return 2;  // message + Rescan
    auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
    return app->getLibrary().size();
}

// Data source, question 2: What's in this cell? The collection view calls this for each
// item it needs, and hands you the frame; you just fill a cell rather than position it.
std::shared_ptr<CollectionViewCell> LibraryViewController::cellForItemAtIndex(
        size_t index, Rect frame) {
    auto cell = std::make_shared<CollectionViewCell>(frame);
    std::string text;
    if (this->libraryIsEmpty()) {
        auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
        if (!app) return cell;
        if (index == 0) {
            // Above, we used the _LS call to get a localized string. Here, we use _LF
            // to use a localized formatted string. Same syntax, but you can also insert
            // things into your string. This is position aware if you have several args:
            // "This is item {0} of {1}" and "Of {1} items, this is {0}" both work.
            text = _LF("library.empty", "No music found in {0}", app->getEngine().libraryLocationHint());
        } else {
            text = _LS("library.rescan", "Rescan");
        }
    } else {
        auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
        text = app->getLibrary()[index].title;
    }
    cell->accessibilityIdentifier = text;
    auto label = std::make_shared<LabelView>(MakeRect(4, (frame.size.height - 8) / 2, frame.size.width - 8, 8), text);
    cell->addSubview(label);
    return cell;
}

// Delegate hooks for d-pad focus. A CollectionViewCell draws no focus indicator on its own.
// That's deliberate: it leaves the look to you. On a TFT like this, inverting looks great.
// On an e-paper screen, you might want to show a smaller indicator beside the selected item,
// to minimize ghosting. Regardless, Focus calls these as the selection moves between rows,
// and in our case we invert the focused one...
void LibraryViewController::didFocusItemAtIndex(CollectionView*, size_t, CollectionViewCell& cell) {
    setCellInverted(cell, true);
}

// ...and restore its normal state when it loses focus.
void LibraryViewController::didUnfocusItemAtIndex(CollectionView*, size_t, CollectionViewCell& cell) {
    setCellInverted(cell, false);
}

// Delegate hook: a row was activated via tap or d-pad SELECT. Start the music, and push
// the now-playing screen onto the navigation stack — that gives us the drill-down and a
// back button without managing either ourselves.
void LibraryViewController::didSelectItemAtIndex(size_t index) {
    auto app = std::static_pointer_cast<PlayerApp>(this->application.lock());
    if (!app) return;
    if (this->libraryIsEmpty()) {
        if (index == 1) {  // Rescan remounts storage and re-reads
            app->rescanLibrary();
            this->reloadData();  // re-ask the data source and rebuild the rows
        }
        return;
    }
    app->playTrackAtIndex(index);
    if (auto nav = this->getNavigationController()) {
        nav->pushViewController(std::make_shared<NowPlayingViewController>(app));
    }
}
