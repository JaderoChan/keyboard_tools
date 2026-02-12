#include <cstdio>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <keyboard_tools/keyboard_tools.hpp>
#include <keyboard_tools/key_utility.hpp>

std::atomic<bool> shouldClose{false};
std::condition_variable shouldCloseCv;

using namespace kbdt;

static bool eventHandler(KeyEvent event)
{
    Key key = keyFromNativeKey(event.nativeKey);
    switch (key)
    {
        case Key_A:
        case Key_B:
        case Key_C:
        case Key_D:
            return false;
        case Key_Esc:
            shouldClose = true;
            shouldCloseCv.notify_one();
            return true;
        default:
            return true;
    }
}

int main()
{
    if (!isEventPropagationBlockSupported())
    {
        printf("This platform does not support blocking key event propagation.");
        exit(1);
    }

    auto& kbdtMgr = KeyboardToolsManager::getInstance();

    int rc = kbdtMgr.run();
    if (rc != KBDT_RC_SUCCESS)
    {
        printf("Failed to run the keyboard tools. Error code: %d\n", rc);
        exit(1);
    }

    rc = kbdtMgr.setEventHandler(&eventHandler);
    if (rc != KBDT_RC_SUCCESS)
    {
        printf("Failed to set the event handler. Error code: %d\n", rc);
        exit(1);
    }

    printf("The key A,B,C,D has been diable.\n");
    printf("Press ESC to exit!\n\n");

    std::mutex dummyMtx;
    std::unique_lock<std::mutex> locker(dummyMtx);
    shouldCloseCv.wait(locker, [&]() { return shouldClose.load(); });

    kbdtMgr.stop();

    printf("Exit successfully!\n");
    return 0;
}
