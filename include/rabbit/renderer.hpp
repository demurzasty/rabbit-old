#pragma once 

#include "system.hpp"
#include "graphics_device.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "texture_cube.hpp"
#include "asset_manager.hpp"

#include <memory>

namespace rb {
    class renderer : public system {
    public:
        renderer(graphics_device& graphics_device, asset_manager& asset_manager);

        void draw(registry& registry, graphics_device& graphics_device);

    private:
        graphics_device& _graphics_device;
        std::shared_ptr<shader> _forward;
        std::shared_ptr<shader> _irradiance;
        std::shared_ptr<shader> _brdf;
        std::shared_ptr<shader> _prefilter;
        std::shared_ptr<shader> _skybox;
        std::shared_ptr<buffer> _matrices_buffer;
        std::shared_ptr<texture_cube> _skybox_texture;
        std::shared_ptr<mesh> _cube;
        std::shared_ptr<mesh> _quad;
    };
}
