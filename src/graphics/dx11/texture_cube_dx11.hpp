#pragma once 

#include <rabbit/graphics/texture_cube.hpp>

#include <d3d11.h>

#include <map>

namespace rb {
	class texture_cube_dx11 : public texture_cube {
	public:
		texture_cube_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const texture_cube_desc& desc);

		~texture_cube_dx11();

		void update(texture_cube_face face, const span<const std::uint8_t>& pixels, const vec4i& rect) override;

		ID3D11Texture2D* texture() const;

		ID3D11ShaderResourceView* shader_resource_view() const;

		ID3D11SamplerState* sampler() const;

		ID3D11RenderTargetView* render_target(texture_cube_face face, int level) const;

	private:
		ID3D11Device* _device = nullptr;
		ID3D11DeviceContext* _context = nullptr;
		ID3D11Texture2D* _texture = nullptr;
		ID3D11ShaderResourceView* _shader_resource_view = nullptr;
		ID3D11SamplerState* _sampler_state = nullptr;
		mutable std::map<texture_cube_face, std::map<std::uint8_t, ID3D11RenderTargetView*>> _render_targets;
	};
}