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

        void set_backbuffer_size(const vec2i& size) override;

        vec2i backbuffer_size() const override;

        void set_clip_rect(const vec4i& clip_rect) override;

        void set_render_target(const std::shared_ptr<texture>& render_target) override;

        void bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) override;

        void draw(topology topology, const std::shared_ptr<buffer>& vertex_buffer, const std::shared_ptr<shader>& shader) override;

    private:
        window& _window;

        GLuint _vao = 0;
    };
}