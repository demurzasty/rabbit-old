#pragma once 

#include "system.hpp"
#include "graphics_device.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "texture_cube.hpp"
#include "asset_manager.hpp"
#include "vec3.hpp"

#include <memory>

namespace rb {
    class renderer : public system {
    public:
        renderer(graphics_device& graphics_device, asset_manager& asset_manager);

        void draw(registry& registry, graphics_device& graphics_device);

    private:
        struct matrices_buffer_data {
            mat4f world;
            mat4f view;
            mat4f projection;
        };

        struct object_data {
            vec3f diffuse;
            float roughness;
            float metallic;
            int bitfield;
        };

        struct light_data {
            vec3f direction;
            float intensity;
            vec3f color;
        };

        struct camera_data {
            vec3f position;
        };

        struct irradiance_data {
            int cube_face = 0;
        };

        struct prefilter_data {
            int cube_face = 0;
            float roughness = 1.0f;
        };

    private:
        void _generate_irradiance_map();

        void _generate_prefilter_map();
        
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
        std::shared_ptr<texture_cube> _irradiance_map;
        std::shared_ptr<texture_cube> _prefilter_map;
        std::shared_ptr<texture> _lut_map;
        std::shared_ptr<buffer> _irradiance_buffer;
        std::shared_ptr<buffer> _prefilter_buffer;
        std::shared_ptr<buffer> _object_buffer;
        std::shared_ptr<buffer> _light_buffer;
        std::shared_ptr<buffer> _camera_buffer;
    };
}
