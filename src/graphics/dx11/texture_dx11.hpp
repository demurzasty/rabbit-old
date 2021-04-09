#pragma once 

#include <rabbit/texture.hpp>

#include <d3d11.h>

namespace rb {
	class texture_dx11 : public texture {
	public:
		texture_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const texture_desc& desc);

		~texture_dx11();

		void update(const span<const std::uint8_t>& pixels, const vec4i& rect) override;

		ID3D11Texture2D* texture() const;

		ID3D11ShaderResourceView* shader_resource_view() const;

		ID3D11SamplerState* sampler() const;

		ID3D11RenderTargetView* render_target() const;

	private:
		ID3D11Device* _device = nullptr;
		ID3D11DeviceContext* _context = nullptr;
		ID3D11Texture2D* _texture = nullptr;
		ID3D11ShaderResourceView* _shader_resource_view = nullptr;
		ID3D11SamplerState* _sampler_state = nullptr;
		ID3D11RenderTargetView* _render_target = nullptr;
	};
}