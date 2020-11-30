#pragma once 

#include <rabbit/mouse.hpp>

#include <glfw/glfw3.h>

namespace rb {
    class mouse_sdl2 : public mouse {
    public:
        mouse_sdl2(std::shared_ptr<window> window);

        void refresh() override;

        vec2i position() override;

        float wheel() override;

        bool is_button_down(mouse_button button) override;

        bool is_button_up(mouse_button button) override;

        bool is_button_pressed(mouse_button button) override;

        bool is_button_released(mouse_button button) override;

    private:
        std::shared_ptr<window> _window;
        Uint32 _state = 0;
        Uint32 _last_state = 0;
        vec2i _mouse_position = { 0, 0 };
    };
}
