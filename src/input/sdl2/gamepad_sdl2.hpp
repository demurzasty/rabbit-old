#pragma once 

#include <rabbit/input/gamepad.hpp>

#include <SDL.h>

namespace rb {
    class gamepad_sdl2 : public gamepad {
    public:
        gamepad_sdl2();

        void refresh() override;

        bool is_button_down(gamepad_player player, gamepad_button button) override;

        bool is_button_up(gamepad_player player, gamepad_button button) override;

        bool is_button_pressed(gamepad_player player, gamepad_button button) override;

        bool is_button_released(gamepad_player player, gamepad_button button) override;

        float axis(gamepad_player player, gamepad_axis axis) override;

    private:
        SDL_GameController* _controllers[4];
        Uint8 _state[4][SDL_CONTROLLER_BUTTON_MAX];
        Uint8 _last_state[4][SDL_CONTROLLER_BUTTON_MAX];
    };
}
