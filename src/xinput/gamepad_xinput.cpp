#include "gamepad_xinput.hpp"

#include <rabbit/enum.hpp>

#include <cstring>
#include <map>

#include <objbase.h>

#pragma comment(lib, "Xinput9_1_0.lib")

using namespace rb;

static std::map<gamepad_button, WORD> gamepad_button_map = {
	{ gamepad_button::dpad_up, XINPUT_GAMEPAD_DPAD_UP },
	{ gamepad_button::dpad_down, XINPUT_GAMEPAD_DPAD_DOWN },
	{ gamepad_button::dpad_left, XINPUT_GAMEPAD_DPAD_LEFT },
	{ gamepad_button::dpad_right, XINPUT_GAMEPAD_DPAD_RIGHT },
	{ gamepad_button::start, XINPUT_GAMEPAD_START },
	{ gamepad_button::back, XINPUT_GAMEPAD_BACK },
	{ gamepad_button::left_thumb, XINPUT_GAMEPAD_LEFT_THUMB },
	{ gamepad_button::right_thumb, XINPUT_GAMEPAD_RIGHT_THUMB },
	{ gamepad_button::left_bumper, XINPUT_GAMEPAD_LEFT_SHOULDER },
	{ gamepad_button::right_bumper, XINPUT_GAMEPAD_RIGHT_SHOULDER },
	{ gamepad_button::a, XINPUT_GAMEPAD_A },
	{ gamepad_button::b, XINPUT_GAMEPAD_B },
	{ gamepad_button::x, XINPUT_GAMEPAD_X },
	{ gamepad_button::y, XINPUT_GAMEPAD_Y }
};

gamepad_xinput::gamepad_xinput()
    : _last_state(), _state() {
	CoInitialize(NULL);
}

void gamepad_xinput::refresh() {
    std::memcpy(_last_state, _state, sizeof(_last_state));
    for (auto i = 0; i < XUSER_MAX_COUNT; ++i) {
        XInputGetState(i, &_state[i]);
    }
}

bool gamepad_xinput::is_button_down(gamepad_player player, gamepad_button button) {
	return _state[enum_size(player)].Gamepad.wButtons & gamepad_button_map[button];
}

bool gamepad_xinput::is_button_up(gamepad_player player, gamepad_button button) {
	return _state[enum_size(player)].Gamepad.wButtons & gamepad_button_map[button];
}

bool gamepad_xinput::is_button_pressed(gamepad_player player, gamepad_button button) {
	return (_state[enum_size(player)].Gamepad.wButtons & gamepad_button_map[button]) &&
		((_last_state[enum_size(player)].Gamepad.wButtons & gamepad_button_map[button]) == 0);
}

bool gamepad_xinput::is_button_released(gamepad_player player, gamepad_button button) {
	return ((_state[enum_size(player)].Gamepad.wButtons & gamepad_button_map[button]) == 0) &&
		(_last_state[enum_size(player)].Gamepad.wButtons & gamepad_button_map[button]);
}

float gamepad_xinput::axis(gamepad_player player, gamepad_axis axis) {
	switch (axis) {
		case gamepad_axis::left_x:
			return _state[enum_size(player)].Gamepad.sThumbLX / 32768.0f;
		case gamepad_axis::left_y:
			return _state[enum_size(player)].Gamepad.sThumbLY / 32768.0f;
		case gamepad_axis::right_x:
			return _state[enum_size(player)].Gamepad.sThumbRX / 32768.0f;
		case gamepad_axis::right_y:
			return _state[enum_size(player)].Gamepad.sThumbRY / 32768.0f;
		case gamepad_axis::left_trigger:
			return _state[enum_size(player)].Gamepad.bLeftTrigger / 255.0f;
		case gamepad_axis::right_trigger:
			return _state[enum_size(player)].Gamepad.bRightTrigger / 255.0f;
	}
	return 0.0f;
}
