#include <keyboard_tools_details.hpp>

#include <atomic>

#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGEvent.h>

#include <uuid.hpp>
#include "event_converter.hpp"

namespace kbdt
{

namespace details
{

// Return code for failed operation.
#define KBDT_RC_FAIL       -1

std::atomic<KeyEventHandler> eventHandler{nullptr};
static std::atomic<CFRunLoopRef> runLoop{NULL};

static CGEventRef keyboardTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* data)
{
    const auto tag = CGEventGetIntegerValueField(event, kCGEventSourceUserData);
    if (tag == uuid())
        return event;

    KeyEvent keyEvent = keyEventFromCGEvent(type, event);
    const auto handler = eventHandler.load();
    if (handler && !handler(keyEvent))
            return NULL;
    return event;
}

int initialize()
{
    return KBDT_RC_SUCCESS;
}

int stopWork()
{
    auto loop = runLoop.load();
    if (loop)
        CFRunLoopStop(loop);
    return KBDT_RC_SUCCESS;
}

int setEventHandler(KeyEventHandler handler)
{
    eventHandler = handler;
    return KBDT_RC_SUCCESS;
}

void work()
{
    auto currentRunLoop = CFRunLoopGetCurrent();
    if (!currentRunLoop)
    {
        setRunFail(KBDT_RC_FAIL);
        return;
    }

    runLoop = currentRunLoop;

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
        NULL);

    if (!eventTap)
    {
        runLoop = NULL;
        setRunFail(KBDT_RC_FAIL);
        return;
    }

    CFRunLoopSourceRef runLoopSource =
        CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (!runLoopSource)
    {
        CFRelease(eventTap);
        runLoop = NULL;
        setRunFail(KBDT_RC_FAIL);
        return;
    }

    CFRunLoopAddSource(runLoop.load(), runLoopSource, kCFRunLoopDefaultMode);
    CGEventTapEnable(eventTap, true);

    setRunSuccess();
    CFRunLoopRun();

    CFRelease(eventTap);
    CFRelease(runLoopSource);

    eventHandler = nullptr;
    runLoop = NULL;
}

} // namespace details

} // namespace kbdt
