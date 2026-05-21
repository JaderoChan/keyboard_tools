#include <keyboard_tools_details.hpp>

#include <string>

#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/input.h>

#include "global_uinput.hpp"

namespace kbt
{

namespace details
{

static bool isKeyboardDevice(int fd)
{
    unsigned long evBits = 0;
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), &evBits) == -1)
        return false;
    if ((evBits & (1 << EV_KEY)) == 0)
        return false;

    char keyBits[KEY_MAX / 8 + 1];
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) == -1)
        return false;

    uint32_t checkedKeys[] = { KEY_0, KEY_A, KEY_SPACE, KEY_ESC };
    size_t checkedCount = sizeof(checkedKeys) / sizeof(uint32_t);
    for (size_t i = 0; i < checkedCount; ++i)
    {
        if ((keyBits[checkedKeys[i] / 8] & (1 << checkedKeys[i] % 8)) == 0)
            return false;
    }
    return true;
}

KeyState getKeyState(uint32_t nativeKey)
{
    if (nativeKey >= KEY_MAX)
        return KS_RELEASED;

    DIR* dir = opendir("/dev/input/");
    if (!dir)
        return KS_RELEASED;

    struct dirent* ent;
    KeyState result = KS_RELEASED;

    while ((ent = readdir(dir)) != nullptr)
    {
        std::string name = ent->d_name;
        if (name.rfind("event", 0) != 0)
            continue;

        std::string path = std::string("/dev/input/") + name;
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd == -1)
            continue;

        if (isInstanceKbdUInput(fd) || !isKeyboardDevice(fd))
        {
            close(fd);
            continue;
        }

        uint8_t keyStateBits[KEY_MAX / 8 + 1] = {};
        if (ioctl(fd, EVIOCGKEY(sizeof(keyStateBits)), keyStateBits) != -1)
        {
            if (keyStateBits[nativeKey / 8] & (1 << (nativeKey % 8)))
                result = KS_PRESSED;
        }
        close(fd);

        if (result == KS_PRESSED)
            break;
    }

    closedir(dir);
    return result;
}

} // namespace details

} // namespace kbt
