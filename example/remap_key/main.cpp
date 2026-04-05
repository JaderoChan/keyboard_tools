#include <cstdio>
#include <condition_variable>
#include <mutex>

#include <keyboard_tools/keyboard_tools.hpp>
#include <keyboard_tools/key_utility.hpp>

bool shouldClose = false;
std::mutex shouldCloseMtx;
std::condition_variable shouldCloseCv;

using namespace kbt;

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
        {
            std::lock_guard<std::mutex> locker(shouldCloseMtx);
            shouldClose = true;
        }
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
        printf("Block event propagation is not supported on this platform!\n");
        return 1;
    }

    auto& eventHookService = EventHookService::getInstance();

    int rc = eventHookService.run();
    if (rc != KBT_RC_SUCCESS)
    {
        printf("Failed to run the keyboard event hook service. Error code: %d.\n", rc);
        exit(1);
    }

    rc = eventHookService.setEventHandler(&eventHandler);
    if (rc != KBT_RC_SUCCESS)
    {
        printf("Failed to set the event handler. Error code: %d.\n", rc);
        exit(1);
    }

    printf("The key A has been remapped to B and the key B has been remapped to A.\n");
    printf("Press ESC to exit!\n\n");

    {
        std::unique_lock<std::mutex> locker(shouldCloseMtx);
        shouldCloseCv.wait(locker, []() { return shouldClose; });
    }

    eventHookService.stop();

    printf("Exit successfully!\n");
    return 0;
}
