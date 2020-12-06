#pragma once 

#include <rabbit/graphics_device.hpp>

#include <volk.h>

namespace rb {
    class graphics_device_vk : public graphics_device {
    public:
        graphics_device_vk(const config& config, std::shared_ptr<window> window);

        ~graphics_device_vk();

        std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

        std::shared_ptr<buffer> make_buffer(const buffer_desc& buffer_desc) override;

        void clear(const color& color) override;

        void present() override;

        void set_blend_state(const blend_state& blend_state) override;

        void set_depth_test(bool depth_test) override;

        void set_view_matrix(const mat4f& view) override;

        void set_projection_matrix(const mat4f& projection) override;

        void set_world_matrix(const mat4f& world) override;

        void set_backbuffer_size(const vec2i& size) override;

        vec2i backbuffer_size() const override;

        void set_clip_rect(const vec4i& clip_rect) override;

        void set_render_target(const std::shared_ptr<texture>& render_target) override;

        void draw(topology topology, const span<const vertex>& vertices) override;

        void draw(topology topology, std::shared_ptr<buffer> buffer) override;

        void draw(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) override;

        void draw(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer) override;

        void draw_textured(topology topology, const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer, const std::shared_ptr<texture>& texture) override;

    private:
        void update_vertex_buffer(const span<const vertex>& vertices);

        void update_index_buffer(const span<const std::uint32_t>& indices);

    private:
        std::shared_ptr<window> _window;

        mat4f _view = mat4f::identity();
        mat4f _projection = mat4f::identity();
        mat4f _world = mat4f::identity();

        VkInstance _instance;
    };
}