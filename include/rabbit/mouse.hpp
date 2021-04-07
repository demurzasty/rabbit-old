#pragma once 

#include "config.hpp"
#include "vec2.hpp"
#include "mouse_button.hpp"
#include "window.hpp"
#include "container.hpp"

#include <memory>

namespace rb {
    class mouse {
    public:
		/**
		 * @brief Install mouse implementation to dependency container.
		 */
        static std::shared_ptr<mouse> resolve(container& container);

        virtual ~mouse() = default;

        virtual void refresh() = 0;

        virtual vec2i position() = 0;

        virtual float wheel() = 0;

        virtual bool is_button_down(mouse_button button) = 0;

        virtual bool is_button_up(mouse_button button) = 0;

        virtual bool is_button_pressed(mouse_button button) = 0;

        virtual bool is_button_released(mouse_button button) = 0;
    };

    std::shared_ptr<mouse> make_mouse(const config& config, std::shared_ptr<window> window);
}