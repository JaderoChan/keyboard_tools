#include <keyboard_tools_details.hpp>

#include <windows.h>

namespace kbt
{

namespace details
{

KeyState getKeyState(uint32_t nativeKey)
{
    SHORT state = GetAsyncKeyState(static_cast<int>(nativeKey));
    return (state & 0x8000) ? KS_PRESSED : KS_RELEASED;
}

} // namespace details

} // namespace kbt
