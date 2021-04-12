#pragma once 

#include "../core/config.hpp"
#include "keycode.hpp"
#include "../platform/window.hpp"
#include "../core/container.hpp"

#include <memory>
#include <string>

namespace rb {
    class keyboard {
    public:
		/**
		 * @brief Install keyboard implementation to dependency container.
		 */
        static void install(installer<keyboard>& installer);

        virtual ~keyboard() = default;

        virtual void refresh() = 0;

        virtual bool is_key_down(keycode key) = 0;

        virtual bool is_key_up(keycode key) = 0;

        virtual bool is_key_pressed(keycode key) = 0;

        virtual bool is_key_released(keycode key) = 0;

        virtual const std::string& input_text() const = 0;
    };
}
