#pragma once 

#include "enum.hpp"
#include "config.hpp"
#include "gamepad_button.hpp"
#include "gamepad_axis.hpp"
#include "container.hpp"

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
        static std::shared_ptr<gamepad> resolve(container& container);

        virtual ~gamepad() = default;

        virtual void refresh() = 0;

        virtual bool is_button_down(gamepad_player player, gamepad_button button) = 0;

        virtual bool is_button_up(gamepad_player player, gamepad_button button) = 0;

        virtual bool is_button_pressed(gamepad_player player, gamepad_button button) = 0;

        virtual bool is_button_released(gamepad_player player, gamepad_button button) = 0;

        virtual float axis(gamepad_player player, gamepad_axis axis) = 0;
    };

    std::shared_ptr<gamepad> make_gamepad(const config& config);
}
