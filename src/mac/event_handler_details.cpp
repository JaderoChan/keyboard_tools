#include <keyboard_tools_details.hpp>

#include <atomic>

#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGEvent.h>

#include <uuid.hpp>
#include "event_converter.hpp"

namespace kbt
{

namespace details
{

// Return code for failed operation.
#define KBT_RC_FAIL       -1

std::atomic<KeyEventHandler> eventHandler{nullptr};
static std::atomic<CFRunLoopRef> runLoop{nullptr};

static CGEventRef keyboardTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* data)
{
    const auto tag = CGEventGetIntegerValueField(event, kCGEventSourceUserData);
    if (tag == uuid())
        return event;

    KeyEvent keyEvent = keyEventFromCGEvent(type, event);
    const auto handler = eventHandler.load();
    if (handler && !handler(keyEvent))
            return nullptr;
    return event;
}

int initialize()
{
    return KBT_RC_SUCCESS;
}

int stopWork()
{
    auto loop = runLoop.load();
    if (loop)
        CFRunLoopStop(loop);
    return KBT_RC_SUCCESS;
}

int setEventHandler(KeyEventHandler handler)
{
    eventHandler.store(handler);
    return KBT_RC_SUCCESS;
}

void work()
{
    auto currentRunLoop = CFRunLoopGetCurrent();
    if (!currentRunLoop)
    {
        setRunFail(KBT_RC_FAIL);
        return;
    }

    runLoop.store(currentRunLoop);

    CGEventMask eventMask =
        CGEventMaskBit(kCGEventKeyDown) |
        CGEventMaskBit(kCGEventKeyUp) |
        CGEventMaskBit(kCGEventFlagsChanged);

    CFMachPortRef eventTap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        eventMask,
        &keyboardTapCallback,
        nullptr);

    if (!eventTap)
    {
        runLoop = nullptr;
        setRunFail(KBT_RC_FAIL);
        return;
    }

    CFRunLoopSourceRef runLoopSource =
        CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (!runLoopSource)
    {
        CFRelease(eventTap);
        runLoop.store(nullptr);
        setRunFail(KBT_RC_FAIL);
        return;
    }

    CFRunLoopAddSource(runLoop.load(), runLoopSource, kCFRunLoopDefaultMode);
    CGEventTapEnable(eventTap, true);

    setRunSuccess();
    CFRunLoopRun();

    CFRelease(eventTap);
    CFRelease(runLoopSource);

    eventHandler.store(nullptr);
    runLoop.store(nullptr);
}

} // namespace details

} // namespace kbdt
