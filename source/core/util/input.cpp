// ============================================================================

/// @brief Input utility

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "log.h"

export module RayBench.Util:Input;

import std;

namespace raybench::util
{

/// @brief Input utility
export class Input
{
public:

    /// @brief Key code
    enum class KeyCode : int
    {
        F1 = VK_F1,
        F2 = VK_F2,
        F3 = VK_F3,
        F4 = VK_F4,
        F5 = VK_F5,
        F6 = VK_F6,
        F7 = VK_F7,
        F8 = VK_F8,
        F9 = VK_F9,
        F10 = VK_F10,
        F11 = VK_F11,
        F12 = VK_F12,
    };

    // ------------------------------------------------------------------------

    /// @brief Check if a key is currently pressed
    ///
    /// @param key Key code to check
    /// @return True if the key is pressed, false otherwise
    static bool IsKeyPressed (KeyCode key)
    {
        return (GetAsyncKeyState (static_cast<int>(key)) & 0x8000) != 0;
    }

    // ------------------------------------------------------------------------

    /// @brief Check if a key was just pressed
    ///
    /// @param key Key code to check
    /// @param was_pressed Reference that tracks the previous state of the key
    /// @return True if the key was just pressed, false otherwise
    static bool IsKeyJustPressed (KeyCode key, bool& was_pressed)
    {
        bool is_pressed = IsKeyPressed (key);
        bool just_pressed = is_pressed && !was_pressed;
        was_pressed = is_pressed;
        return just_pressed;
    }
};

}

// ----------------------------------------------------------------------------