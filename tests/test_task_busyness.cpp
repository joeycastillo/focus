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

class BusyTestApplication : public Application {
public:
    BusyTestApplication(const std::shared_ptr<Window>& window) : Application(window) {}
    void setup() override {}

    /// Run one pass of the task loop, matching Application::run()'s removal logic.
    void runOneIteration() {
        auto application = this->shared_from_this();
        for (int i = 0; i < (int)this->tasks.size(); i++) {
            if (this->tasks[i]->run(application)) {
                this->tasks.erase(this->tasks.begin() + i);
                i--;
            }
        }
    }
};

std::shared_ptr<BusyTestApplication> makeApp() {
    auto window = std::make_shared<Window>(nullptr, MakeSize(100, 100));
    return std::make_shared<BusyTestApplication>(window);
}

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

TEST(any_task_busy_false_when_no_task_is_busy) {
    auto app = makeApp();
    app->addTask(std::make_shared<PlainTask>());
    app->addTask(std::make_shared<FlaggedTask>());
    ASSERT_FALSE(app->anyTaskBusy());
}

TEST(any_task_busy_tracks_the_busy_flag) {
    auto app = makeApp();
    auto flagged = std::make_shared<FlaggedTask>();
    app->addTask(std::make_shared<PlainTask>());
    app->addTask(flagged);
    flagged->busy = true;
    ASSERT_TRUE(app->anyTaskBusy());
    flagged->busy = false;
    ASSERT_FALSE(app->anyTaskBusy());
}

TEST(any_task_busy_callable_from_within_run) {
    // A policy-shaped task queries busyness mid-iteration.
    class PollingPolicyTask : public Task {
    public:
        bool sawBusy = false;
        bool run(std::shared_ptr<Application> application) override {
            this->sawBusy = application->anyTaskBusy();
            return false;
        }
    };
    auto app = makeApp();
    auto policy = std::make_shared<PollingPolicyTask>();
    auto flagged = std::make_shared<FlaggedTask>();
    app->addTask(policy);
    app->addTask(flagged);
    flagged->busy = true;
    app->runOneIteration();
    ASSERT_TRUE(policy->sawBusy);
}

TEST(removed_task_no_longer_counts_as_busy) {
    auto app = makeApp();
    auto flagged = std::make_shared<FlaggedTask>();
    app->addTask(flagged);
    flagged->busy = true;
    flagged->removeOnNextRun = true;
    ASSERT_TRUE(app->anyTaskBusy());
    app->runOneIteration();
    ASSERT_FALSE(app->anyTaskBusy());
}
