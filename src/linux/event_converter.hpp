#ifndef KEYBOARD_TOOLS_LINUX_EVENT_CONVERTER_HPP
#define KEYBOARD_TOOLS_LINUX_EVENT_CONVERTER_HPP

#include <linux/input.h>

#include <keyboard_tools/types.hpp>

namespace kbdt
{

namespace details
{

struct input_event keyEventToInputEvent(const KeyEvent& event);

KeyEvent keyEventFromInputEvent(const struct input_event& ie);

} // namespace details

} // namespace kbdt

#endif // !KEYBOARD_TOOLS_LINUX_EVENT_CONVERTER_HPP
