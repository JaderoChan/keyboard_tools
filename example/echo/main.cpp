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

    switch (event.type)
    {
        case KET_PRESSED:
        {
            printf("Key pressed: %s\n", keyToStr(key));
            if (key == Key_Esc)
            {
                {
                    std::lock_guard<std::mutex> locker(shouldCloseMtx);
                    shouldClose = true;
                }
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
    if (rc != KBT_RC_SUCCESS)
    {
        printf("Failed to run the keyboard tools. Error code: %d.\n", rc);
        exit(1);
    }

    rc = kbdtMgr.setEventHandler(&eventHandler);
    if (rc != KBT_RC_SUCCESS)
    {
        printf("Failed to set the event handler. Error code: %d.\n", rc);
        exit(1);
    }

    printf("Press ESC to exit!\n\n");

    {
        std::unique_lock<std::mutex> locker(shouldCloseMtx);
        shouldCloseCv.wait(locker, []() { return shouldClose; });
    }

    kbdtMgr.stop();

    printf("Exit successfully!\n");
    return 0;
}
