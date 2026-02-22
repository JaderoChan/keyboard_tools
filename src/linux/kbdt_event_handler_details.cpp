#include <keyboard_tools_details.hpp>

#include <string>
#include <atomic>
#include <thread>
#include <list>
#include <mutex>

#include <errno.h>
#include <dirent.h>         // dirent
#include <fcntl.h>          // open
#include <unistd.h>         // read, write, close
#include <poll.h>           // poll
#include <sys/eventfd.h>    // eventfd
#include <sys/inotify.h>    // inotify
#include <sys/stat.h>       // stat
#include <linux/input.h>    // input_event

#include "event_converter.hpp"
#include "global_uinput.hpp"

namespace kbdt
{

namespace details
{

#define EVDEV_DIR "/dev/input/"

static KeyEventHandler eventHandler = nullptr;

// Only used to indicate exiting the worker thread and setting the event handler.
struct WorkEvent
{
    enum Type
    {
        END,
        SET_HANDLER
    };

    Type type;
    KeyEventHandler handler;
};

static std::list<WorkEvent> workEvents;
static std::mutex workEventsMtx;

// watchedFds layout:
// [0] work event fd: see also work event.
// [1] inotify fd: Detect evdev device changes.
// [2..] opened evdev keyboard fds.
// Reserve 2 pollfd for 'work event fd' and 'inotify fd'.
static std::vector<struct pollfd> watchedFds(2, pollfd{-1, 0, 0});
static std::vector<std::string> evdevNames;

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

    uint32_t checkedKeys[] = {
        KEY_0, KEY_A, KEY_SPACE, KEY_ESC
    };
    size_t checkedCount = sizeof(checkedKeys) / sizeof(uint32_t);
    for (size_t i = 0; i < checkedCount; ++i)
    {
        if ((keyBits[checkedKeys[i] / 8] & (1 << checkedKeys[i] % 8)) == 0)
            return false;
    }
    return true;
}

static int emitWorkEvent(const WorkEvent& workEvent)
{
    // Push work event first, then wake woker by writing increments counter to the event fd.
    {
        std::lock_guard<std::mutex> locker(workEventsMtx);
        workEvents.push_back(workEvent);
    }

    uint64_t wakeup = 1;
    ssize_t wsize = write(watchedFds[0].fd, &wakeup, 8);
    if (wsize != 8)
    {
        std::lock_guard<std::mutex> locker(workEventsMtx);
        workEvents.pop_back();
        return errno;
    }
    return KBDT_RC_SUCCESS;
}

static int setupWorkEventFd()
{
    int workEventFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (workEventFd == -1)
        return errno;

    struct pollfd pollFd;
    pollFd.fd = workEventFd;
    pollFd.events = POLLIN;
    pollFd.revents = 0;
    watchedFds[0] = pollFd;

    return KBDT_RC_SUCCESS;
}

static void cleanupWorkEventFd()
{
    close(watchedFds[0].fd);
    watchedFds[0].fd = -1;
}

static int setupInotifyFd()
{
    int inotifyFd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    if (inotifyFd == -1)
        return errno;

    int wd = inotify_add_watch(inotifyFd, EVDEV_DIR, IN_CREATE | IN_DELETE);
    if (wd == -1)
    {
        close(inotifyFd);
        return errno;
    }

    struct pollfd pollFd;
    pollFd.fd = inotifyFd;
    pollFd.events = POLLIN;
    pollFd.revents = 0;
    watchedFds[1] = pollFd;

    return KBDT_RC_SUCCESS;
}

static void cleanupInotifyFd()
{
    close(watchedFds[1].fd);
    watchedFds[1].fd = -1;
}

static void addEvdevFd(const std::string& evdevName)
{
    std::string filepath = EVDEV_DIR + evdevName;

    // Only accept character devices.
    if (!isCharacterDevice(filepath))
        return;

    int evdevFd = open(filepath.c_str(), O_RDONLY | O_NONBLOCK);
    if (evdevFd == -1)
        return;

    // Only accept keyboard-like devices.
    if (!isKeyboardDevice(evdevFd))
    {
        close(evdevFd);
        return;
    }

    if (isInstanceKbdUInput(evdevFd))
    {
        close(evdevFd);
        return;
    }

    struct pollfd pollFd;
    pollFd.fd = evdevFd;
    pollFd.events = POLLIN;
    pollFd.revents = 0;

    watchedFds.push_back(pollFd);
    evdevNames.push_back(evdevName);
}

static void removeEvdevFd(const std::string& evdevName)
{
    for (size_t i = 0; i < evdevNames.size(); ++i)
    {
        if (evdevName == evdevNames[i])
        {
            int idx = i + 2;
            close(watchedFds[idx].fd);
            watchedFds.erase(watchedFds.begin() + idx);
            evdevNames.erase(evdevNames.begin() + i);
            return;
        }
    }
}

static int setupEvdevFds()
{
    DIR* dir = opendir(EVDEV_DIR);
    if (!dir)
        return errno;

    struct dirent* ent = readdir(dir);
    while (ent)
    {
        addEvdevFd(ent->d_name);
        ent = readdir(dir);
    }
    closedir(dir);

    return KBDT_RC_SUCCESS;
}

static void cleanupEvdevFds()
{
    for (auto it = watchedFds.begin() + 2; it != watchedFds.end();)
    {
        close(it->fd);
        it = watchedFds.erase(it);
    }
    evdevNames.clear();
}

static int setupAll()
{
    int rc = setupWorkEventFd();
    if (rc != KBDT_RC_SUCCESS)
        return rc;

    rc = setupInotifyFd();
    if (rc != KBDT_RC_SUCCESS)
    {
        cleanupWorkEventFd();
        return rc;
    }

    rc = setupEvdevFds();
    if (rc != KBDT_RC_SUCCESS)
    {
        cleanupInotifyFd();
        cleanupWorkEventFd();
        return rc;
    }

    return KBDT_RC_SUCCESS;
}

static void cleanupAll()
{
    cleanupWorkEventFd();
    cleanupInotifyFd();
    cleanupEvdevFds();

    eventHandler = nullptr;

    {
        // Clear work event list.
        std::lock_guard<std::mutex> locker(workEventsMtx);
        workEvents.clear();
    }
}

static void handleDevicesChanged()
{
    alignas(struct inotify_event) char buf[4096];

    while (true)
    {
        ssize_t rsize = read(watchedFds[1].fd, buf, sizeof(buf));
        if (rsize > 0)
        {
            size_t off = 0;
            while (off + sizeof(struct inotify_event) <= static_cast<size_t>(rsize))
            {
                auto* ev = reinterpret_cast<struct inotify_event*>(buf + off);

                if ((ev->mask & IN_CREATE) && ev->len > 0)
                    addEvdevFd(ev->name);
                else if ((ev->mask & IN_DELETE) && ev->len > 0)
                    removeEvdevFd(ev->name);

                off += sizeof(struct inotify_event) + ev->len;
            }
        }
        else if (rsize < 0 && errno == EINTR)
        {
            // Interrupted by signal, retry read.
            continue;
        }
        else
        {
            // Non-blocking fd: EAGAIN/EWOULDBLOCK means drained.
            break;
        }
    }
}

static void handleInputEvent(int evdevFd)
{
    struct input_event ie;
    while (true)
    {
        ssize_t rsize = read(evdevFd, &ie, sizeof(struct input_event));
        if (rsize == sizeof(struct input_event))
        {
            if (ie.type == EV_KEY)
                eventHandler(keyEventFromInputEvent(ie));
        }
        else if (rsize < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            // Non-blocking fd drained.
            break;
        }
    }
}

int initialize()
{
    return setupAll();
}

int stopWork()
{
    return emitWorkEvent({WorkEvent::END});
}

int setEventHandler(KeyEventHandler handler)
{
    return emitWorkEvent({WorkEvent::SET_HANDLER, handler});
}

void work()
{
    setRunSuccess();
    bool shouldClose = false;
    while (!shouldClose)
    {
        int ret = poll(watchedFds.data(), watchedFds.size(), -1);
        if (ret == -1)
            continue;

        // Handle work events.
        struct pollfd pollFd = watchedFds[0];
        if (pollFd.revents & POLLIN)
        {
            uint64_t counting;
            ssize_t rsize = read(pollFd.fd, &counting, 8);
            if (rsize == 8)
            {
                for (uint64_t i = 0; i < counting; ++i)
                {
                    WorkEvent workEvent;
                    bool hasEvent = false;
                    {
                        std::lock_guard<std::mutex> locker(workEventsMtx);
                        if (!workEvents.empty())
                        {
                            workEvent = workEvents.front();
                            workEvents.pop_front();
                            hasEvent = true;
                        }
                    }

                    if (!hasEvent)
                        break;

                    switch (workEvent.type)
                    {
                        case WorkEvent::END:
                            shouldClose = true;
                            break;
                        case WorkEvent::SET_HANDLER:
                            eventHandler = workEvent.handler;
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        // Handle input devices changed event.
        pollFd = watchedFds[1];
        if (pollFd.revents & POLLIN)
            handleDevicesChanged();

        // Handle input devices events.
        for (size_t i = 2; i < watchedFds.size(); ++i)
        {
            pollFd = watchedFds[i];
            if (pollFd.revents & POLLIN)
                handleInputEvent(pollFd.fd);
        }
    }

    cleanupAll();
}

} // namespace details

} // namespace kbdt
