#include <keyboard_tools_details.hpp>

#include "global_uinput.hpp"

namespace kbt
{

namespace details
{

size_t sendEvents(const std::vector<KeyEvent>& events)
{
    return globalKbdUInputInstance().sendEvents(events);
}

} // namespace details

} // namespace kbt
