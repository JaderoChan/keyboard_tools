#ifndef KEYBOARD_TOOLS_KBD_UINPUT_HPP
#define KEYBOARD_TOOLS_KBD_UINPUT_HPP

#include <string>
#include <vector>
#include <mutex>

#include <keyboard_tools/types.hpp>

namespace kbdt
{

namespace details
{

class KbdUInput
{
public:
    KbdUInput() = default;

    KbdUInput(const std::string& uinputName, uint16_t vendor, uint16_t product, uint16_t version);

    ~KbdUInput();

    bool setup(const std::string& uinputName, uint16_t vendor, uint16_t product, uint16_t version);

    void cleanup();

    int fd() const;

    bool isSetup() const;

    size_t sendEvents(const std::vector<KeyEvent>& events);

private:
    int fd_ = -1;
    mutable std::mutex fdWriteMtx_;
};

} // namespace details

} // namespac kbdt

#endif // !KEYBOARD_TOOLS_KBD_UINPUT_HPP
