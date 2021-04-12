#pragma once 

#include <rabbit/input/mouse.hpp>

namespace rb {
    class mouse_dummy : public mouse {
    public:
        mouse_dummy() = default;
        
        void refresh() override;

        vec2i position() override;

        float wheel() override;

        bool is_button_down(mouse_button button) override;

        bool is_button_up(mouse_button button) override;

        bool is_button_pressed(mouse_button button) override;

        bool is_button_released(mouse_button button) override;
    };
}
