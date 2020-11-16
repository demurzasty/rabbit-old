#pragma once 

#include <rabbit/gamepad.hpp>

#if RB_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <Xinput.h>

namespace rb {
    class gamepad_xinput : public gamepad {
    public:
        gamepad_xinput();

        void refresh() override;

        bool is_button_down(gamepad_player player, gamepad_button button) override;

        bool is_button_up(gamepad_player player, gamepad_button button) override;

        bool is_button_pressed(gamepad_player player, gamepad_button button) override;

        bool is_button_released(gamepad_player player, gamepad_button button) override;

        float axis(gamepad_player player, gamepad_axis axis) override;

    private:
        XINPUT_STATE _last_state[XUSER_MAX_COUNT];
        XINPUT_STATE _state[XUSER_MAX_COUNT];
    };
}
