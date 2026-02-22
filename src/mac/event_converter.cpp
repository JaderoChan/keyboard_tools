#include "event_converter.hpp"

#include <Carbon/Carbon.h>

namespace kbdt
{

namespace details
{

static CGEventFlags modifierMaskForKeyCode(CGKeyCode keyCode)
{
    switch (keyCode)
    {
        case kVK_Shift:
        case kVK_RightShift:
            return kCGEventFlagMaskShift;
        case kVK_Control:
        case kVK_RightControl:
            return kCGEventFlagMaskControl;
        case kVK_Option:
        case kVK_RightOption:
            return kCGEventFlagMaskAlternate;
        case kVK_Command:
        case kVK_RightCommand:
            return kCGEventFlagMaskCommand;
        case kVK_CapsLock:
            return kCGEventFlagMaskAlphaShift;
        case kVK_Function:
            return kCGEventFlagMaskSecondaryFn;
        default:
            return 0;
    }
}

CGEventRef keyEventToCGEvent(const KeyEvent& event, uint64_t uuid)
{
    CGEventRef cgEvent = NULL;

    CGKeyCode keyCode = (CGKeyCode) event.nativeKey;
    switch (event.type)
    {
        case KET_PRESSED:
            cgEvent = CGEventCreateKeyboardEvent(NULL, keyCode, true);
            break;
        case KET_RELEASED:
            cgEvent = CGEventCreateKeyboardEvent(NULL, keyCode, false);
            break;
        default:
            return NULL;
    }

    CGEventSetIntegerValueField(cgEvent, kCGEventSourceUserData, (int64_t) uuid);
    return cgEvent;
}

KeyEvent keyEventFromCGEvent(CGEventType cgEventType, CGEventRef cgEvent)
{
    KeyEvent event;

    event.nativeKey = (uint32_t) CGEventGetIntegerValueField(cgEvent, kCGKeyboardEventKeycode);
    switch (cgEventType)
    {
        case kCGEventKeyDown:
            event.type = KET_PRESSED;
            break;
        case kCGEventKeyUp:
            event.type = KET_RELEASED;
            break;
        case kCGEventFlagsChanged:
        {
            CGEventFlags mask = modifierMaskForKeyCode((CGKeyCode) event.nativeKey);
            if (mask != 0)
            {
                CGEventFlags flags = CGEventGetFlags(cgEvent);
                event.type = (flags & mask) ? KET_PRESSED : KET_RELEASED;
            }
            break;
        }
        default:
            break;
    }

    return event;
}

} // namespace details

} // namespace kbdt
