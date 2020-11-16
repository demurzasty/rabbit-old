#pragma once 

#include <rabbit/keyboard.hpp>

#include <cstdint>

namespace rb {
    class keyboard_win32 : public keyboard {
    public:
        keyboard_win32();

        void refresh() override;

        bool is_key_down(keycode key) override;

        bool is_key_up(keycode key) override;

        bool is_key_pressed(keycode key) override;

        bool is_key_released(keycode key) override;

    private:
        std::uint8_t _last_state[256];
        std::uint8_t _state[256];
    };
}
