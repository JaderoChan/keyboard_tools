#ifndef KEYBOARD_TOOLS_GLOBAL_UINPUT_HPP
#define KEYBOARD_TOOLS_GLOBAL_UINPUT_HPP

#include <cstdint>

#include "kbd_uinput.hpp"

namespace kbdt
{

namespace details
{

// 0x1003 is a magic number (What this is? My name JC: J-10 C-03)
constexpr uint16_t UINPUT_VENDOR = 0x1003;
constexpr const char* KBD_UINPUT_NAME = "JC Keyboard Tools UInput";

KbdUInput& globalKbdUInputInstance();

// Check if the fd corresponds to the event device created by the global KbdUInput instance.
bool isInstanceKbdUInput(int fd);

} // namespace details

} // namespace kbdt

#endif // !KEYBOARD_TOOLS_GLOBAL_UINPUT_HPP
