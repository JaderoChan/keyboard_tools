#include <keyboard_tools_details.hpp>

#include <string>
#include <vector>
#include <mutex>

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <linux/input.h>

#include "global_uinput.hpp"

namespace kbt
{

namespace details
{

#define EVDEV_DIR "/dev/input/"

static bool isCharacterDevice(const std::string& filepath)
{
    struct stat st;
    if (stat(filepath.c_str(), &st) == -1)
        return false;
    return S_ISCHR(st.st_mode);
}

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

    int32_t checkedKeys[] = {KEY_0, KEY_A, KEY_SPACE, KEY_ESC};
    size_t checkedCount = sizeof(checkedKeys) / sizeof(int32_t);
    for (size_t i = 0; i < checkedCount; ++i)
    {
        if ((keyBits[checkedKeys[i] / 8] & (1 << checkedKeys[i] % 8)) == 0)
            return false;
    }
    return true;
}

struct KbdFdCache
{
    KbdFdCache()
    {
        inotifyFd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if (inotifyFd != -1)
            inotify_add_watch(inotifyFd, EVDEV_DIR, IN_CREATE | IN_DELETE);
    }

    ~KbdFdCache()
    {
        for (int fd : fds)
            close(fd);
        if (inotifyFd != -1)
            close(inotifyFd);
    }

    // Drain inotify events, returns true if any device change was detected.
    bool drainInotify()
    {
        if (inotifyFd == -1)
            return false;

        alignas(struct inotify_event) char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
        bool changed = false;
        while (read(inotifyFd, buf, sizeof(buf)) > 0)
            changed = true;
        return changed;
    }

    void refresh()
    {
        for (int fd : fds)
            close(fd);
        fds.clear();
        needsRefresh = false;

        DIR* dir = opendir(EVDEV_DIR);
        if (!dir)
            return;

        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr)
        {
            std::string path = std::string(EVDEV_DIR) + ent->d_name;
            if (!isCharacterDevice(path))
                continue;

            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd == -1)
                continue;

            if (isInstanceKbdUInput(fd) || !isKeyboardDevice(fd))
            {
                close(fd);
                continue;
            }

            fds.push_back(fd);
        }

        closedir(dir);
    }

    std::vector<int> fds;
    int inotifyFd = -1;
    bool needsRefresh = true;
    std::mutex mtx;
};

KeyState getKeyState(int32_t nativeKey)
{
    static KbdFdCache cache;
    std::lock_guard<std::mutex> lock(cache.mtx);

    if (cache.needsRefresh || cache.drainInotify())
        cache.refresh();

    uint8_t keyStateBits[KEY_MAX / 8 + 1] = {0};
    for (int fd : cache.fds)
    {
        int rc = ioctl(fd, EVIOCGKEY(sizeof(keyStateBits)), keyStateBits);
        if (rc == -1)
        {
            if (errno == ENODEV || errno == ENXIO)
                cache.needsRefresh = true;
            continue;
        }

        if (keyStateBits[nativeKey / 8] & (1 << (nativeKey % 8)))
            return KS_PRESSED;
    }

    return KS_RELEASED;
}

} // namespace details

} // namespace kbt
