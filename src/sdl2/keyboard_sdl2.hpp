#pragma once 

#include <rabbit/keyboard.hpp>
#include <rabbit/window.hpp>

#include <SDL.h>

namespace rb {
    class keyboard_sdl2 : public keyboard {
    public:
        keyboard_sdl2(std::shared_ptr<window> window);

        void refresh() override;

        bool is_key_down(keycode key) override;

        bool is_key_up(keycode key) override;

        bool is_key_pressed(keycode key) override;

        bool is_key_released(keycode key) override;

        const std::string& input_text() const override;

    private:
        std::shared_ptr<window> _window;
        Uint8 _last_state[SDL_NUM_SCANCODES];
        Uint8 _state[SDL_NUM_SCANCODES];
    };
}
