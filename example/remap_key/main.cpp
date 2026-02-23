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
        // Block event propagation and send new events to achieve key remapping.
        case Key_A:
            sendEvent(KeyEvent{event.type, keyToNativeKey(Key_B)});
            return false;
        case Key_B:
            sendEvent(KeyEvent{event.type, keyToNativeKey(Key_A)});
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
    if (!isSupportBlockEventPropagation())
    {
        printf("Block event propagation is not supported on this platform.\n");
        return 1;
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

    printf("The key A has been remapped to B and the key B has been remapped to A.\n");
    printf("Press ESC to exit!\n\n");

    std::mutex dummyMtx;
    std::unique_lock<std::mutex> dummyLocker(dummyMtx);
    shouldCloseCv.wait(dummyLocker, [&]() { return shouldClose.load(); });

    kbdtMgr.stop();

    printf("Exit successfully!\n");
    return 0;
}
