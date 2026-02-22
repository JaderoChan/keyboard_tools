#include "global_uinput.hpp"

#include <cstring>

#include <sys/ioctl.h>
#include <linux/input.h>

#include <uuid.hpp>

#define UINT32_LOW16(x)     (uint16_t) ((x) & 0xFFFF)
#define UINT32_HIGH16(x)    (uint16_t) (((x) >> 16) & 0xFFFF)

namespace kbdt
{

namespace details
{

KbdUInput& globalKbdUInputInstance()
{
    // Attach uinput uuid via uinput product id and version.
    static KbdUInput instance(
        KBD_UINPUT_NAME,
        UINPUT_VENDOR,
        UINT32_LOW16((uint32_t) uuid()),
        UINT32_HIGH16((uint32_t) uuid()));
    return instance;
}

bool isInstanceKbdUInput(int fd)
{
    char name[256] = {0};
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
        return false;

    struct input_id id = {0};
    if (ioctl(fd, EVIOCGID, &id) < 0)
        return false;

    return strcmp(name, KBD_UINPUT_NAME) == 0 &&
           id.vendor == UINPUT_VENDOR &&
           id.product == UINT32_LOW16((uint32_t) uuid()) &&
           id.version == UINT32_HIGH16((uint32_t) uuid());
}

} // namespace details

} // namespace kbdt
