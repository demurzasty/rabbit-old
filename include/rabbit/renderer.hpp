#pragma once

#include "system.hpp"
#include "material.hpp"
#include "resource_heap.hpp"
#include "buffer.hpp"
#include "mat4.hpp"

#include <memory>

namespace rb {
    class renderer : public system {
    public:
        renderer(graphics_device& graphics_device);

        void draw(registry& registry, graphics_device& graphics_device) override;
    
    private:
        struct alignas(16) matrices {
            mat4f proj;
            mat4f view;
            mat4f world;
        };

    private:
        std::shared_ptr<buffer> _matrices_buffer;
        std::shared_ptr<material> _forward_material;
    };
}
