#pragma once 

#include <rabbit/input/keyboard.hpp>

namespace rb {
    class keyboard_dummy : public keyboard {
    public:
        void refresh() override;

        bool is_key_down(keycode key) override;

        bool is_key_up(keycode key) override;

        bool is_key_pressed(keycode key) override;

        bool is_key_released(keycode key) override;
    };
}
