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

    switch (event.type)
    {
        case KET_PRESSED:
        {
            printf("Key pressed: %s\n", keyToStr(key));
            if (key == Key_Esc)
            {
                shouldClose = true;
                shouldCloseCv.notify_one();
            }
            break;
        }
        case KET_RELEASED:
        {
            printf("Key released: %s\n", keyToStr(key));
            break;
        }
        default:
            break;
    }

    return true;
}

int main()
{
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

    printf("Press ESC to exit!\n\n");

    std::mutex dummyMtx;
    std::unique_lock<std::mutex> dummyLocker(dummyMtx);
    shouldCloseCv.wait(dummyLocker, [&]() { return shouldClose.load(); });

    kbdtMgr.stop();

    printf("Exit successfully!\n");
    return 0;
}
