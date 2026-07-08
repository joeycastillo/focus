#include "viewcontrollers/AboutViewController.hpp"

#include "Application.hpp"
#include "TextView.hpp"
#include "Focus.hpp"

using namespace focus;

// A page or two of scrollable text. \x0E/\x0F are the SO/SI emphasis codes.
static const char* kAboutText =
    "Focus on PyGamer\n"
    "SAMD51 / 160x128 ST7735\n"
    "\n"
    "This page demonstrates \x0E" "ScrollView\x0F and \x0E" "TextView\x0F: "
    "a long document laid out once, rendered lazily, and scrolled by "
    "setting a discrete offset. Push the joystick down to scroll; at the "
    "top of the page, pushing up moves focus to the back button.\n"
    "\n"
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
    "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim "
    "ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
    "aliquip ex ea commodo consequat. Duis aute irure dolor in "
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
    "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
    "culpa qui officia deserunt mollit anim id est laborum.\n"
    "\n"
    "Sed ut perspiciatis unde omnis iste natus error sit voluptatem "
    "accusantium doloremque laudantium, totam rem aperiam, eaque ipsa "
    "quae ab illo inventore veritatis et quasi architecto beatae vitae "
    "dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit "
    "aspernatur aut odit aut fugit, sed quia consequuntur magni dolores "
    "eos qui ratione voluptatem sequi nesciunt.\n"
    "\n"
    "At vero eos et accusamus et iusto odio dignissimos ducimus qui "
    "blanditiis praesentium voluptatum deleniti atque corrupti quos "
    "dolores et quas molestias excepturi sint occaecati cupiditate non "
    "provident, similique sunt in culpa qui officia deserunt mollitia "
    "animi, id est laborum et dolorum fuga. Et harum quidem rerum facilis "
    "est et expedita distinctio.\n";

bool ScrollingPageView::handleEvent(Event event) {
    if (this->scrollView &&
        (event.type == FOCUS_EVENT_DIRECTION_UP || event.type == FOCUS_EVENT_DIRECTION_DOWN)) {
        Point offset = this->scrollView->getScrollOffset();
        int step = this->scrollView->getFrame().size.height / 2;
        int target = (event.type == FOCUS_EVENT_DIRECTION_DOWN) ? offset.y + step
                                                                : offset.y - step;
        this->scrollView->setScrollOffset(MakePoint(offset.x, target));
        // At an edge the clamped offset doesn't move; decline so the event
        // bubbles (up at the top walks focus to the nav bar's back button).
        if (this->scrollView->getScrollOffset().y != offset.y) return true;
    }
    return View::handleEvent(event);
}

AboutViewController::AboutViewController(std::shared_ptr<Application> application) : ViewController(application) {
    this->setTitle("About");
}

void AboutViewController::createView() {
    this->view = std::make_shared<View>(RectZero);
    this->view->setOpaque(false);

    this->page = std::make_shared<ScrollingPageView>(RectZero);
    this->scrollView = std::make_shared<ScrollView>(RectZero);
    this->textView = std::make_shared<TextView>(RectZero, kAboutText);
    this->page->setScrollView(this->scrollView);

    this->scrollView->addSubview(this->textView);
    this->page->addSubview(this->scrollView);
    this->view->addSubview(this->page);
}

void AboutViewController::viewDidLayoutSubviews() {
    Rect frame = this->view->getFrame();
    if (frame.size.width <= 0 || frame.size.height <= 0) return;

    this->page->setFrame(MakeRect(0, 0, frame.size.width, frame.size.height));
    this->scrollView->setFrame(MakeRect(0, 0, frame.size.width, frame.size.height));

    // The canonical ScrollView + TextView sizing recipe.
    int width = frame.size.width - 8;  // 4px side margins
    int height = this->textView->heightForWidth(width);
    this->textView->setFrame(MakeRect(4, 0, width, height));
    this->scrollView->setContentSize(MakeSize(frame.size.width, height));
}
