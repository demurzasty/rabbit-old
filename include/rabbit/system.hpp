#pragma once 

#include "entity.hpp"
#include "graphics_device.hpp"

namespace rb {
    struct system {
        virtual ~system() = default;

        virtual void initialize(registry& registry);

        virtual void variable_update(registry& registry, float elapsed_time);

        virtual void fixed_update(registry& registry, float fixed_time);

        virtual void draw(registry& registry, graphics_device& graphics_device);
    };
}
