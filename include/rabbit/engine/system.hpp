#pragma once

#include "entity.hpp"
#include "../graphics/graphics_device.hpp"

namespace rb {
    class system {
    public:
        virtual ~system() = default;

        virtual void initialize(registry& registry);

        virtual void update(registry& registry, float elapsed_time);

        virtual void fixed_update(registry& registry, float elapsed_time);

        virtual void draw(registry& registry, graphics_device& graphics_device);

        virtual void present(registry& registry, graphics_device& graphics_device);
    };
}
