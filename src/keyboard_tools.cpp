#include <keyboard_tools/keyboard_tools.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "keyboard_tools_details.hpp"

namespace kbt
{

enum RunningState : uint8_t
{
    RS_FREE,
    RS_RUNNING,
    RS_TERMINATE
};

static std::mutex runStopMtx;
static std::thread workerThread;
static RunningState runningState = RS_FREE;
static std::mutex runningStateMtx;
static std::condition_variable runningStateCv;
static int runningRc = -1;

static void threadWork()
{
    details::work();
    {
        std::lock_guard<std::mutex> locker(runningStateMtx);
        // If the work exits normally (running state is `RS_RUNNING`), set the state to `RS_FREE`.
        // If the work exits due to an error (running state is `RS_TERMINATE`), preserve the error state.
        if (runningState == RS_RUNNING)
            runningState = RS_FREE;
    }
    runningStateCv.notify_one();
}

EventHookService& EventHookService::getInstance()
{
    static EventHookService instance;
    return instance;
}

int EventHookService::run()
{
    std::lock_guard<std::mutex> runStopLocker(runStopMtx);

    if (isRunning())
        return KBT_RC_SUCCESS;

    int rc = details::initialize();
    if (rc != KBT_RC_SUCCESS)
        return rc;

    workerThread = std::thread(&threadWork);

    {
        std::unique_lock<std::mutex> locker(runningStateMtx);
        // Wait for the worker thread to set the running state.
        runningStateCv.wait(locker, []() { return runningState != RS_FREE; });
        if (runningState == RS_TERMINATE)
            runningState = RS_FREE;
        rc = runningRc;
    }

    if (rc != KBT_RC_SUCCESS)
    {
        if (workerThread.joinable())
            workerThread.join();
    }

    return rc;
}

int EventHookService::stop()
{
    std::lock_guard<std::mutex> runStopLocker(runStopMtx);

    if (!isRunning())
        return KBT_RC_SUCCESS;

    int rc = details::stopWork();
    if (rc != KBT_RC_SUCCESS)
        return rc;

    if (workerThread.joinable())
        workerThread.join();

    return KBT_RC_SUCCESS;
}

int EventHookService::setEventHandler(KeyEventHandler handler)
{
    return details::setEventHandler(handler);
}

bool EventHookService::isRunning() noexcept
{
    std::lock_guard<std::mutex> locker(runningStateMtx);
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
#else
    return false;
#endif
}

namespace details
{

void setRunSuccess()
{
    {
        std::lock_guard<std::mutex> locker(runningStateMtx);
        runningRc = KBT_RC_SUCCESS;
        runningState = RS_RUNNING;
    }
    runningStateCv.notify_one();
}

void setRunFail(int errorCode)
{
    std::lock_guard<std::mutex> locker(runningStateMtx);
    runningRc = errorCode;
    runningState = RS_TERMINATE;
}

} // namespace deatils

} // namespace kbt
