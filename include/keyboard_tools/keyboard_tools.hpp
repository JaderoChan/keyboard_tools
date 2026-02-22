// Repository: https://github.com/JaderoChan/keyboard_tools
// Author: 頔珞JaderoChan

// MIT License
//
// Copyright (c) 2025 頔珞JaderoChan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef KEYBOARD_TOOLS_HPP
#define KEYBOARD_TOOLS_HPP

#include <vector>

#include <keyboard_tools/config.hpp>

#include "defines.hpp"
#include "types.hpp"

namespace kbdt
{

class KEYBOARD_TOOLS_API KeyboardToolsManager
{
public:
    static KeyboardToolsManager& getInstance();

    /**
     * @brief Run the keyboard tools service.
     * @return `KBDT_RC_SUCCESS` on success, error code on failure.
     * @note thread-safe
     * @sa stop()
     */
    int run();

    /**
     * @brief Stop the keyboard tools service.
     * @return `KBDT_RC_SUCCESS` on success, error code on failure.
     * @note Can be executed safely in threads other than worker threads.
     * @attention Do not call this function from within the event handler thread.
     * @sa run()
     */
    int stop();

    /**
     * @brief Set the key event handler callback function.
     * @param handler Function for handling key events.
     * @return `KBDT_RC_SUCCESS` on success, error code on failure.
     * @note It only be set while service is running.
     * @note thread-safe
     */
    int setEventHandler(KeyEventHandler handler);

    /**
     * @brief Check if the keyboard tools service is running.
     * @return True if the service is running, false otherwise.
     * @note thread-safe
     */
    bool isRunning() noexcept;

private:
    KeyboardToolsManager() = default;
    ~KeyboardToolsManager() = default;
    KeyboardToolsManager(const KeyboardToolsManager&) = delete;
    KeyboardToolsManager& operator=(const KeyboardToolsManager&) = delete;
};

/**
 * @brief Send multiple key events.
 * @param events List of key events to send.
 * @return Number of events successfully sent.
 * @note Events sent through this function will not be received by the event handler.
 * @note Ensure that event sending is atomic, that is, event lists sent by different threads will not be interleaved.
 * @note thread-safe
 * @sa sendEvent()
 */
size_t sendEvents(const std::vector<KeyEvent>& events);

/**
 * @brief Send a single key event.
 * @param event Key event to send.
 * @return True if the event been sent successfully, false otherwise.
 * @note Events sent through this function will not be received by the event handler.
 * @note thread-safe
 * @sa sendEvents()
 */
bool sendEvent(const KeyEvent& event);

/**
 * @brief Check whether blocking the propagation of key events is supported.
 * @return True if blocking is supported, false otherwise.
 * @note thread-safe
 */
bool isSupportBlockEventPropagation() noexcept;

} // namespace kbdt

#endif // !KEYBOARD_TOOLS_HPP
