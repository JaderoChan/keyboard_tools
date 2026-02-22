#include "event_converter.hpp"

namespace kbdt
{

namespace details
{

INPUT keyEventToInput(const KeyEvent& event, uint64_t uuid)
{
    INPUT input = {0};

    input.type = INPUT_KEYBOARD;
    input.ki.wVk = (WORD) event.nativeKey;
    input.ki.dwExtraInfo = (ULONG_PTR) uuid;
    switch (event.type)
    {
        case KET_RELEASED:
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            break;
        case KET_PRESSED:
            input.ki.dwFlags = 0;
            break;
        default:
            break;
    }

    return input;
}

KeyEvent keyEventFromParam(WPARAM wParam, LPARAM lParam)
{
    KeyEvent event;

    switch (wParam)
    {
        case WM_KEYDOWN:    // Fallthrough
        case WM_SYSKEYDOWN:
            event.type = KET_PRESSED;
            break;
        case WM_KEYUP:      // Fallthrough
        case WM_SYSKEYUP:
            event.type = KET_RELEASED;
            break;
        default:
            break;
    }
    KBDLLHOOKSTRUCT* kbdllhs = (KBDLLHOOKSTRUCT*) lParam;
    event.nativeKey = kbdllhs->vkCode;

    return event;
}

} // namespace details

} // namespace kbdt
