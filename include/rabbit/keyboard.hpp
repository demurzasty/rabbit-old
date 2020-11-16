#pragma once 

#include "config.hpp"
#include "keycode.hpp"

#include <memory>

namespace rb {
    class keyboard {
    public:
        virtual ~keyboard() = default;

        virtual void refresh() = 0;

        virtual bool is_key_down(keycode key) = 0;

        virtual bool is_key_up(keycode key) = 0;

        virtual bool is_key_pressed(keycode key) = 0;

        virtual bool is_key_released(keycode key) = 0;
    };

    std::shared_ptr<keyboard> make_keyboard(const config& config);
}
