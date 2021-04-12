#pragma once 

#include <rabbit/input/gamepad.hpp>

namespace rb {
    class gamepad_dummy : public gamepad {
    public:
        gamepad_dummy() = default;
        
        void refresh() override;

        bool is_button_down(gamepad_player player, gamepad_button button) override;

        bool is_button_up(gamepad_player player, gamepad_button button) override;

        bool is_button_pressed(gamepad_player player, gamepad_button button) override;

        bool is_button_released(gamepad_player player, gamepad_button button) override;

        float axis(gamepad_player player, gamepad_axis axis) override;
    };
}
