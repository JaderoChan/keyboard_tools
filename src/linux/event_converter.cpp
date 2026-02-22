#include "event_converter.hpp"

namespace kbdt
{

namespace details
{

struct input_event keyEventToInputEvent(const KeyEvent& event)
{
    struct input_event ie = {0};
    ie.type = EV_KEY;
    ie.code = event.nativeKey;
    ie.value = (event.type == KET_PRESSED ? 1 : 0);
    return ie;
}

KeyEvent keyEventFromInputEvent(const struct input_event& ie)
{
    KeyEvent event;
    event.type = (ie.value == 0 ? KET_RELEASED : KET_PRESSED);
    event.nativeKey = (uint32_t) ie.code;
    return event;
}

} // namespace details

} // namespace kbdt
