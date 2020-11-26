#include "gamepad_sdl2.hpp"
#include "gamecontrollerdb_sdl2.hpp"

#include <rabbit/enum.hpp>
#include <rabbit/exception.hpp>

#include <algorithm>

using namespace rb;

static SDL_GameControllerAxis axes[] = {
    SDL_CONTROLLER_AXIS_LEFTX,
    SDL_CONTROLLER_AXIS_LEFTY,
    SDL_CONTROLLER_AXIS_RIGHTX,
    SDL_CONTROLLER_AXIS_RIGHTY,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
};

static SDL_GameControllerButton buttons[] = {
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,

    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK,

    SDL_CONTROLLER_BUTTON_BACK,
    SDL_CONTROLLER_BUTTON_START,

    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
};

gamepad_sdl2::gamepad_sdl2()
    : _controllers()
    , _last_state()
    , _state() {
    if (SDL_GameControllerAddMapping(gamecontrollerdb::mapping()) < 0) {
        throw exception{ SDL_GetError() };
    }
}

void gamepad_sdl2::refresh() {
    for (int i = 0; i < 4; ++i) {
        // Detect which controller was disconnected
        if (_controllers[i] && !SDL_IsGameController(i)) {
            SDL_GameControllerClose(_controllers[i]);
            _controllers[i] = nullptr;
        }

        // Detect which controller was connected
        if (!_controllers[i] && SDL_IsGameController(i)) {
            _controllers[i] = SDL_GameControllerOpen(i);
        }

        // Refresh state if controller is active
        if (_controllers[i]) {
            // Save last state
            SDL_memcpy(_last_state, _state, sizeof(_last_state));

            // Get current state of controller
            for (auto button : buttons) {
                _state[i][button] = SDL_GameControllerGetButton(_controllers[i], button);
            }
        }
    }
}

bool gamepad_sdl2::is_button_down(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return _state[enum_size(player)][buttons[enum_size(button)]];
    }
    return false;
}

bool gamepad_sdl2::is_button_up(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return !_state[enum_size(player)][buttons[enum_size(button)]];
    }
    return true;
}

bool gamepad_sdl2::is_button_pressed(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return _state[enum_size(player)][buttons[enum_size(button)]] &&
            !_last_state[enum_size(player)][buttons[enum_size(button)]];
    }
    return false;
}

bool gamepad_sdl2::is_button_released(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return !_state[enum_size(player)][buttons[enum_size(button)]] &&
            _last_state[enum_size(player)][buttons[enum_size(button)]];
    }
    return false;
}

float gamepad_sdl2::axis(gamepad_player player, gamepad_axis axis) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return SDL_GameControllerGetAxis(controller, axes[enum_size(axis)]) / 32767.0f;
    }
    return 0.0f;
}
