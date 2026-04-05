#include <keyboard_tools_details.hpp>

#include "global_uinput.hpp"

namespace kbdt
{

namespace details
{

size_t sendEvents(const std::vector<KeyEvent>& events)
{
    return globalKbdUInputInstance().sendEvents(events);
}

} // namespace details

} // namespace kbdt
