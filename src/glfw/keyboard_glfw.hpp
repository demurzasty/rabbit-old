#pragma once 

#include <rabbit/window.hpp>
#include <rabbit/keyboard.hpp>

#include <glfw/glfw3.h>

namespace rb {
    class keyboard_glfw : public keyboard {
    public:
        keyboard_glfw(std::shared_ptr<window> window);

        void refresh() override;

        bool is_key_down(keycode key) override;

        bool is_key_up(keycode key) override;

        bool is_key_pressed(keycode key) override;

        bool is_key_released(keycode key) override;

    private:
        std::shared_ptr<window> _window;
        std::uint8_t _last_state[GLFW_KEY_LAST + 1];
        std::uint8_t _state[GLFW_KEY_LAST + 1];
    };
}
