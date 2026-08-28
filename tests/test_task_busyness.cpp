/*
 * Tests for Task::isBusy() and Application::anyTaskBusy().
 */

#include "test_harness.hpp"
#include "Application.hpp"
#include "Task.hpp"
#include "Window.hpp"

using namespace focus;

namespace {

// Task that never overrides isBusy().
class PlainTask : public Task {
public:
    bool run(std::shared_ptr<Application>) override { return false; }
};

// Task whose busyness and self-removal are settable fields.
class FlaggedTask : public Task {
public:
    bool busy = false;
    bool removeOnNextRun = false;
    bool run(std::shared_ptr<Application>) override { return removeOnNextRun; }
    bool isBusy() const override { return busy; }
};

}  // namespace

TEST(task_isbusy_defaults_to_false) {
    PlainTask task;
    ASSERT_FALSE(task.isBusy());
}

TEST(task_isbusy_override_reflects_state) {
    FlaggedTask task;
    Task& base = task;
    ASSERT_FALSE(base.isBusy());
    task.busy = true;
    ASSERT_TRUE(base.isBusy());
}
