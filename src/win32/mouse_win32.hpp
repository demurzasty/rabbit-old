#pragma once 

#include <rabbit/mouse.hpp>

#include <cstdint>

namespace rb {
    class mouse_win32 : public mouse {
    public:
        mouse_win32(std::shared_ptr<window> window);

        void refresh() override;

        vec2i position() override;

        bool is_button_down(mouse_button button) override;

        bool is_button_up(mouse_button button) override;

        bool is_button_pressed(mouse_button button) override;

        bool is_button_released(mouse_button button) override;

    private:
        std::shared_ptr<window> _window;
        std::uint8_t _last_state[8];
        std::uint8_t _state[8];
    };
}
