#include <cstdio>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <keyboard_tools/keyboard_tools.hpp>
#include <keyboard_tools/key_utility.hpp>

std::atomic<bool> shouldClose{false};
std::condition_variable shouldCloseCv;

using namespace kbdt;

static void inputPi()
{
    static Key pi[] = {
        Key_3, Key_Period, Key_1,
        Key_4, Key_1, Key_5,
        Key_9, Key_2, Key_6
    };
    static size_t size = sizeof(pi) / sizeof(Key);
    static std::vector<KeyEvent> events;
    static bool inited = false;

    if (!inited)
    {
        for (size_t i = 0; i < size; ++i)
        {
            int nativeKey = keyToNativeKey(pi[i]);
            events.push_back({KET_PRESSED, nativeKey});
            events.push_back({KET_RELEASED, nativeKey});
        }

        inited = true;
    }

    sendEvents(events);
}

static bool eventHandler(KeyEvent event)
{
    static Key keySeq[2] = {(Key) 0};

    Key key = keyFromNativeKey(event.nativeKey);
    switch (event.type)
    {
        case KET_PRESSED:
        {
            keySeq[0] = keySeq[1];
            keySeq[1] = key;

            if (key == Key_Esc)
            {
                shouldClose = true;
                shouldCloseCv.notify_one();
            }

            break;
        }
        case KET_RELEASED:
        {
            if (keySeq[0] == Key_P && keySeq[1] == Key_I)
            {
                std::thread th(&inputPi);
                th.detach();
            }
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
    std::unique_lock<std::mutex> locker(dummyMtx);
    shouldCloseCv.wait(locker, [&]() { return shouldClose.load(); });

    kbdtMgr.stop();

    printf("Successfully exit!\n");
    return 0;
}
