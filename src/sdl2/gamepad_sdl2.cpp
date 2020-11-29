#include "gamepad_sdl2.hpp"
#include "gamecontrollerdb_sdl2.hpp"

#include <rabbit/enum.hpp>
#include <rabbit/exception.hpp>

#include <map>
#include <algorithm>

using namespace rb;

static std::map<gamepad_axis, SDL_GameControllerAxis> axes = {
    { gamepad_axis::left_x, SDL_CONTROLLER_AXIS_LEFTX },
    { gamepad_axis::left_y, SDL_CONTROLLER_AXIS_LEFTY },
    { gamepad_axis::right_x, SDL_CONTROLLER_AXIS_RIGHTX },
    { gamepad_axis::right_y, SDL_CONTROLLER_AXIS_RIGHTY },
    { gamepad_axis::left_trigger, SDL_CONTROLLER_AXIS_TRIGGERLEFT },
    { gamepad_axis::right_trigger, SDL_CONTROLLER_AXIS_TRIGGERRIGHT },
};

static std::map<gamepad_button, SDL_GameControllerButton> buttons = {
    { gamepad_button::a, SDL_CONTROLLER_BUTTON_A },
    { gamepad_button::b, SDL_CONTROLLER_BUTTON_B },
    { gamepad_button::x, SDL_CONTROLLER_BUTTON_X },
    { gamepad_button::y, SDL_CONTROLLER_BUTTON_Y },

    { gamepad_button::left_bumper, SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
    { gamepad_button::right_bumper, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
    { gamepad_button::left_thumb, SDL_CONTROLLER_BUTTON_LEFTSTICK },
    { gamepad_button::right_thumb, SDL_CONTROLLER_BUTTON_RIGHTSTICK },

    { gamepad_button::back, SDL_CONTROLLER_BUTTON_BACK },
    { gamepad_button::start, SDL_CONTROLLER_BUTTON_START },

    { gamepad_button::dpad_up, SDL_CONTROLLER_BUTTON_DPAD_UP },
    { gamepad_button::dpad_right, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
    { gamepad_button::dpad_down, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
    { gamepad_button::dpad_left, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
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
            for (auto [button, id] : buttons) {
                _state[i][id] = SDL_GameControllerGetButton(_controllers[i], id);
            }
        }
    }
}

bool gamepad_sdl2::is_button_down(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return _state[enum_size(player)][buttons.at(button)];
    }
    return false;
}

bool gamepad_sdl2::is_button_up(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return !_state[enum_size(player)][buttons.at(button)];
    }
    return true;
}

bool gamepad_sdl2::is_button_pressed(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return _state[enum_size(player)][buttons.at(button)] &&
            !_last_state[enum_size(player)][buttons.at(button)];
    }
    return false;
}

bool gamepad_sdl2::is_button_released(gamepad_player player, gamepad_button button) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return !_state[enum_size(player)][buttons.at(button)] &&
            _last_state[enum_size(player)][buttons.at(button)];
    }
    return false;
}

float gamepad_sdl2::axis(gamepad_player player, gamepad_axis axis) {
    auto controller = _controllers[enum_size(player)];
    if (controller) {
        return SDL_GameControllerGetAxis(controller, axes.at(axis)) / 32767.0f;
    }
    return 0.0f;
}
