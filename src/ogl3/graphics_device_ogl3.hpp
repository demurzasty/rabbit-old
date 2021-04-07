#pragma once 

#include <rabbit/graphics_device.hpp>

#include <GL/glew.h>

#if RB_WINDOWS
#include <GL/wglew.h>
#endif

namespace rb {
    class graphics_device_ogl3 : public graphics_device {
    public:
        graphics_device_ogl3(const config& config, window& window);

        ~graphics_device_ogl3();

        std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

        std::shared_ptr<buffer> make_buffer(const buffer_desc& buffer_desc) override;

        std::shared_ptr<shader> make_shader(const shader_desc& shader_desc) override;

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

        void bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) override;

        void draw(topology topology, const span<const vertex>& vertices) override;

        void draw(topology topology, std::shared_ptr<buffer> buffer) override;

        void draw(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) override;

        void draw(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer) override;

        void draw_textured(topology topology, const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer, const std::shared_ptr<texture>& texture) override;

    private:
        GLuint compile_program(const char* vertex_shader_code, const char* fragment_shader_code) const;

        void update_vertex_buffer(const span<const vertex>& vertices);

        void update_index_buffer(const span<const std::uint32_t>& indices);

    private:
        window& _window;

        mat4f _view = mat4f::identity();
        mat4f _projection = mat4f::identity();
        mat4f _world = mat4f::identity();

        GLuint _vao = 0;
        GLuint _vbo = 0;
        GLuint _ibo = 0;
        GLuint _solid_program = 0;
        GLuint _texture_program = 0;
    };
}