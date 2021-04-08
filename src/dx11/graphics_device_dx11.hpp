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
        graphics_device_dx11(config& config, window& window);

        ~graphics_device_dx11();

        std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

        std::shared_ptr<buffer> make_buffer(const buffer_desc& buffer_desc) override;

        std::shared_ptr<shader> make_shader(const shader_desc& shader_desc) override;

        std::shared_ptr<mesh> make_mesh(const mesh_desc& mesh_desc) override;

        void clear(const color& color) override;

        void present() override;

        void set_blend_state(const blend_state& blend_state) override;

        void set_depth_test(bool depth_test) override;

        void set_backbuffer_size(const vec2i& size) override;

        vec2i backbuffer_size() const override;

        void set_clip_rect(const vec4i& clip_rect) override;

        void set_render_target(const std::shared_ptr<texture>& render_target) override;

        void bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) override;

        void draw(const std::shared_ptr<mesh>& vertex_buffer, const std::shared_ptr<shader>& shader) override;

        ID3D11Device* device() const;

        ID3D11DeviceContext1* device_context() const;

    private:
        window& _window; // keep reference
        ID3D11Device* _device = nullptr;
        ID3D11DeviceContext1* _device_context = nullptr;
        IDXGISwapChain1* _swap_chain = nullptr;
        ID3D11RenderTargetView* _render_target = nullptr;
        ID3D11RenderTargetView* _offscreen_render_target = nullptr;
        ID3D11Texture2D* _depth_stencil_texture = nullptr;
        ID3D11DepthStencilView* _depth_stencil_view = nullptr;
        ID3D11RasterizerState* _rasterizer_state = nullptr;
        D3D11_TEXTURE2D_DESC _depth_desc = {};
        D3D11_DEPTH_STENCIL_VIEW_DESC _depth_stencil_view_desc = {};
        std::map<uint64_t, ID3D11BlendState*> _blend_states;
        bool _depth_test = false;
        bool _vsync;
    };
}
