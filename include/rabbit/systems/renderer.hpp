#pragma once

#include "../engine/system.hpp"
#include "../graphics/material.hpp"
#include "../graphics/shader.hpp"
#include "../graphics/buffer.hpp"
#include "../math/mat4.hpp"
#include "../graphics/shader_data.hpp"

#include <memory>
#include <unordered_map>

namespace rb {
    class renderer : public system {
    public:
        renderer(graphics_device& graphics_device);

        void draw(registry& registry, graphics_device& graphics_device) override;

    private:
        struct alignas(16) camera_data {
            mat4f proj;
            mat4f view;
        };

        struct alignas(16) local_data {
            mat4f world;
        };

        struct alignas(16) material_data {
            vec3f base_color;
            float roughness;
            float metallic;
        };

        struct entity_render_data {
            std::shared_ptr<shader_data> shader_data;
            std::shared_ptr<buffer> local_buffer;
            std::shared_ptr<buffer> material_buffer;
        };

    private:
        std::shared_ptr<shader> _forward_shader;
        std::shared_ptr<buffer> _camera_buffer;
        std::unordered_map<entity, entity_render_data> _entity_render_data;

        std::shared_ptr<texture> _white_texture;
    };
}
