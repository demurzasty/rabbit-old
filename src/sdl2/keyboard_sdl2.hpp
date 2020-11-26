#pragma once 

#include <rabbit/keyboard.hpp>

#include <SDL.h>

namespace rb {
    class keyboard_sdl2 : public keyboard {
    public:
        keyboard_sdl2();

        void refresh() override;

        bool is_key_down(keycode key) override;

        bool is_key_up(keycode key) override;

        bool is_key_pressed(keycode key) override;

        bool is_key_released(keycode key) override;

    private:
        Uint8 _last_state[SDL_NUM_SCANCODES];
        Uint8 _state[SDL_NUM_SCANCODES];
    };
}
