#include <keyboard_tools_details.hpp>

#include <windows.h>

#include <uuid.hpp>
#include "event_converter.hpp"

namespace kbdt
{

namespace details
{

size_t sendEvents(const std::vector<KeyEvent>& events)
{
    auto eventCount = events.size();
    if (eventCount == 0)
        return 0;

    std::vector<INPUT> inputs;
    inputs.reserve(eventCount);
    for (size_t i = 0; i < eventCount; ++i)
        inputs.push_back(keyEventToInput(events[i], uuid()));

    UINT sent = SendInput((UINT) eventCount, inputs.data(), sizeof(INPUT));
    return sent;
}

} // namespace details

} // namespace kbdt
