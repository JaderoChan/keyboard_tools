#include <keyboard_tools_details.hpp>

#include <CoreGraphics/CGEventSource.h>

namespace kbt
{

namespace details
{

KeyState getKeyState(int32_t nativeKey)
{
    bool pressed = CGEventSourceKeyState(
        kCGEventSourceStateCombinedSessionState,
        static_cast<CGKeyCode>(nativeKey));
    return pressed ? KS_PRESSED : KS_RELEASED;
}

} // namespace details

} // namespace kbt
