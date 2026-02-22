#include <keyboard_tools_details.hpp>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CGEvent.h>

#include <uuid.hpp>
#include "event_converter.hpp"

namespace kbdt
{

namespace details
{

size_t sendEvents(const std::vector<KeyEvent>& events)
{
    size_t sent = 0;
    for (size_t i = 0; i < events.size(); ++i)
    {
        auto cgEvent = keyEventToCGEvent(events[i], uuid());
        if (cgEvent)
        {
            CGEventPost(kCGHIDEventTap, cgEvent);
            CFRelease(cgEvent);
            sent++;
        }
    }
    return sent;
}

} // namespace details

} // namespace kbdt
