#pragma once 

#include "system.hpp"
#include "graphics_device.hpp"
#include "shader.hpp"

namespace rb {
    class renderer : public system {
    public:
        renderer(graphics_device& graphics_device);

        void draw(registry& registry, graphics_device& graphics_device);

    private:
        graphics_device& _graphics_device;
        std::shared_ptr<shader> _forward;
    };
}
