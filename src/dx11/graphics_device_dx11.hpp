#pragma once 

#include <rabbit/graphics_device.hpp>
#include <rabbit/window.hpp>

#include <d3d11.h>
#include <d3d11_1.h>

#include <map>
#include <memory>

namespace rb {
    class graphics_device_dx11 : public graphics_device {
    public:
        graphics_device_dx11(const config& config, std::shared_ptr<window> window);

        ~graphics_device_dx11();

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

        void draw(topology topology, std::shared_ptr<buffer> vertex_buffer) override;

        void draw(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) override;

        void draw(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer) override;

        void draw_textured(topology topology, const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const std::shared_ptr<texture>& texture) override;

        void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer, const std::shared_ptr<texture>& texture) override;
        
        ID3D11Device* device() const;

        ID3D11DeviceContext1* device_context() const;

    private:
        void update_vertex_buffer(const span<const vertex>& vertices);

        void update_index_buffer(const span<const std::uint32_t>& indices);

    private:
        struct vertex_shader_data {
            mat4f world = mat4f::identity();
            mat4f view = mat4f::identity();
            mat4f projection = mat4f::identity();
        };

    private:
        std::shared_ptr<window> _window; // keep reference
        ID3D11Device* _device = nullptr;
        ID3D11DeviceContext1* _device_context = nullptr;
        IDXGISwapChain1* _swap_chain = nullptr;
        ID3D11RenderTargetView* _render_target = nullptr;
        ID3D11RenderTargetView* _offscreen_render_target = nullptr;
        ID3D11Texture2D* _depth_stencil_texture = nullptr;
        ID3D11DepthStencilView* _depth_stencil_view = nullptr;
        ID3D11Buffer* _vertex_buffer = nullptr;
        ID3D11Buffer* _index_buffer = nullptr;
        ID3D11Buffer* _vertex_constant_buffer = nullptr;
        ID3D11VertexShader* _vertex_shader = nullptr;
        ID3D11PixelShader* _color_pixel_shader = nullptr;
        ID3D11PixelShader* _texture_pixel_shader = nullptr;
        ID3D11InputLayout* _input_layout = nullptr;
        ID3D11RasterizerState* _rasterizer_state = nullptr;
        vertex_shader_data _vertex_shader_data;
        std::map<uint64_t, ID3D11BlendState*> _blend_states;
        bool _depth_test = false;
    };
}
