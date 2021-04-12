#pragma once 

#include "../core/enum.hpp"
#include "../core/config.hpp"
#include "gamepad_button.hpp"
#include "gamepad_axis.hpp"
#include "../core/container.hpp"

#include <memory>

namespace rb {
    // todo: split into another file
    RB_ENUM(gamepad_player, std::uint8_t, "first", "second", "third", "fourth")
    enum class gamepad_player : std::uint8_t {
        first,
        second,
        third,
        fourth
    };

    class gamepad {
    public:
		/**
		 * @brief Install gamepad implementation to dependency container.
		 */
        static void install(installer<gamepad>& installer);

        virtual ~gamepad() = default;

        virtual void refresh() = 0;

        virtual bool is_button_down(gamepad_player player, gamepad_button button) = 0;

        virtual bool is_button_up(gamepad_player player, gamepad_button button) = 0;

        virtual bool is_button_pressed(gamepad_player player, gamepad_button button) = 0;

        virtual bool is_button_released(gamepad_player player, gamepad_button button) = 0;

        virtual float axis(gamepad_player player, gamepad_axis axis) = 0;
    };
}
