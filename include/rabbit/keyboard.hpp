#pragma once 

#include "config.hpp"
#include "keycode.hpp"
#include "window.hpp"
#include "container.hpp"

#include <memory>
#include <string>

namespace rb {
    class keyboard {
    public:
		/**
		 * @brief Install keyboard implementation to dependency container.
		 */
        static std::shared_ptr<keyboard> resolve(container& container);

        virtual ~keyboard() = default;

        virtual void refresh() = 0;

        virtual bool is_key_down(keycode key) = 0;

        virtual bool is_key_up(keycode key) = 0;

        virtual bool is_key_pressed(keycode key) = 0;

        virtual bool is_key_released(keycode key) = 0;

        virtual const std::string& input_text() const = 0;
    };

    std::shared_ptr<keyboard> make_keyboard(const config& config, std::shared_ptr<window> window);
}
