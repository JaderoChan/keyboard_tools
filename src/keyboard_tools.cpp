#include <keyboard_tools/keyboard_tools.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "keyboard_tools_details.hpp"

namespace kbdt
{

enum RunningState : uint8_t
{
    RS_FREE,
    RS_RUNNING,
    RS_TERMINATE
};

static std::mutex runAndStopMtx;
static std::thread workerThread;
static std::condition_variable runningStateCv;
static std::atomic<RunningState> runningState{RS_FREE};
static std::atomic<int> runningRc{-1};

static void threadWork()
{
    details::work();
    // If the work exits normally (running state is `RS_RUNNING`), set the state to `RS_FREE`.
    // If the work exits due to an error (running state is `RS_TERMINATE`), preserve the error state.
    if (runningState == RS_RUNNING)
        runningState = RS_FREE;
    runningStateCv.notify_one();
}

KeyboardToolsManager& KeyboardToolsManager::getInstance()
{
    static KeyboardToolsManager instance;
    return instance;
}

int KeyboardToolsManager::run()
{
    std::lock_guard<std::mutex> locker(runAndStopMtx);

    if (isRunning())
        return KBDT_RC_SUCCESS;

    int rc = details::initialize();
    if (rc != KBDT_RC_SUCCESS)
        return rc;

    workerThread = std::thread(&threadWork);
    workerThread.detach();

    std::mutex dummyMtx;
    std::unique_lock<std::mutex> dummyLocker(dummyMtx);
    // Wait for the worker thread to set the running state.
    runningStateCv.wait(dummyLocker, [&]() { return runningState != RS_FREE; });
    if (runningState == RS_TERMINATE)
        runningState = RS_FREE;
    rc = runningRc;

    return rc;
}

int KeyboardToolsManager::stop()
{
    std::lock_guard<std::mutex> locker(runAndStopMtx);

    if (!isRunning())
        return KBDT_RC_SUCCESS;

    int rc = details::stopWork();
    if (rc != KBDT_RC_SUCCESS)
        return rc;

    std::mutex dummyMtx;
    std::unique_lock<std::mutex> dummyLocker(dummyMtx);
    // Wait for the worker thread to exit and transition to free state.
    runningStateCv.wait(dummyLocker, [&]() { return runningState == RS_FREE; });

    return KBDT_RC_SUCCESS;
}

int KeyboardToolsManager::setEventHandler(KeyEventHandler handler)
{
    return details::setEventHandler(handler);
}

bool KeyboardToolsManager::isRunning() noexcept
{
    return runningState == RS_RUNNING;
}

size_t sendEvents(const std::vector<KeyEvent>& events)
{
    return details::sendEvents(events);
}

bool sendEvent(const KeyEvent& event)
{
    return sendEvents({event}) == 1;
}

bool isSupportBlockEventPropagation() noexcept
{
#if defined(KEYBOARD_TOOLS_WIN) || defined(KEYBOARD_TOOLS_MAC)
    return true;
#elif defined(KEYBOARD_TOOLS_LINUX) && defined(KEYBOARD_TOOLS_LINUX_EVENT_BLOCKER)
    return true;
#else
    return false;
#endif
}

void setRunSuccess()
{
    runningRc = KBDT_RC_SUCCESS;
    runningState = RS_RUNNING;
    runningStateCv.notify_one();
}

void setRunFail(int errorCode)
{
    runningRc = errorCode;
    runningState = RS_TERMINATE;
}

} // namespace kbdt
