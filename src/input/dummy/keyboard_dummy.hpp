#pragma once 

#include <rabbit/input/keyboard.hpp>

namespace rb {
    class keyboard_dummy : public keyboard {
    public:
        keyboard_dummy() = default;
        
        void refresh() override;

        bool is_key_down(keycode key) override;

        bool is_key_up(keycode key) override;

        bool is_key_pressed(keycode key) override;

        bool is_key_released(keycode key) override;

        const std::string& input_text() const override;
    };
}
