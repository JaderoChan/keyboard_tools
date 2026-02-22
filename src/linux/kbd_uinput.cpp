#include "kbd_uinput.hpp"

#include <cstring>
#include <array>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/input-event-codes.h>

#include "event_converter.hpp"

namespace kbdt
{

namespace details
{

static struct input_event createSyncEvent()
{
    struct input_event ie = {0};

    ie.type = EV_SYN;
    ie.code = SYN_REPORT;
    ie.value = 0;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    return ie;
}

constexpr size_t EVENT_PAIR_SIZE = 2 * sizeof(struct input_event);
static std::array<struct input_event, 2> createEventPair()
{
    std::array<struct input_event, 2> ies = {0};
    ies[1] = createSyncEvent();
    return ies;
}

KbdUInput::KbdUInput(const std::string& uinputName, uint16_t vendor, uint16_t product, uint16_t version)
{
    setup(uinputName, vendor, product, version);
}

KbdUInput::~KbdUInput()
{
    cleanup();
}

bool KbdUInput::setup(const std::string& uinputName, uint16_t vendor, uint16_t product, uint16_t version)
{
    if (isSetup())
        return false;

    constexpr const char* uinputFilepath = "/dev/uinput";
    fd_ = open(uinputFilepath, O_WRONLY | O_NONBLOCK);
    if (fd_ == -1)
        return false;

    struct uinput_setup usetup = {0};
    snprintf(usetup.name, sizeof(usetup.name), "%s", uinputName.c_str());
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = vendor;
    usetup.id.product = product;
    usetup.id.version = version;

    if (ioctl(fd_, UI_DEV_SETUP, &usetup) == -1)
    {
        cleanup();
        return false;
    }

    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) == -1 ||
        ioctl(fd_, UI_SET_EVBIT, EV_SYN) == -1)
    {
        cleanup();
        return false;
    }

    for (size_t i = 0; i <= KEY_MAX; ++i)
    {
        if (ioctl(fd_, UI_SET_KEYBIT, i) == -1)
        {
            cleanup();
            return false;
        }
    }

    if (ioctl(fd_, UI_DEV_CREATE) == -1)
    {
        cleanup();
        return false;
    }

    return true;
}

void KbdUInput::cleanup()
{
    if (isSetup())
    {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
        fd_ = -1;
    }
}

int KbdUInput::fd() const
{
    return fd_;
}

bool KbdUInput::isSetup() const
{
    return fd_ != -1;
}

size_t KbdUInput::sendEvents(const std::vector<KeyEvent>& events)
{
    if (!isSetup())
        return 0;

    std::lock_guard<std::mutex> locker(fdWriteMtx_);

    size_t sent = 0;
    auto ies = createEventPair();

    for (size_t i = 0; i < events.size(); ++i)
    {
        ies[0] = keyEventToInputEvent(events[i]);

        while (true)
        {
            ssize_t wsize = write(fd_, ies.data(), EVENT_PAIR_SIZE);
            if (wsize == EVENT_PAIR_SIZE)
            {
                sent++;
                break;
            }
            else if (wsize < 0 && errno == EINTR)
            {
                // Interrupted by signal, retry write.
                continue;
            }
            else
            {
                // Other errors, stop sending.
                break;
            }
        }
    }

    return sent;
}

} // namespace details

} // namespace kbdt
