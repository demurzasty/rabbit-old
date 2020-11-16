#include "texture_dx11.hpp"

#include <cassert>
#include <map>

using namespace rb;

static std::map<texture_format, DXGI_FORMAT> formats = {
	{ texture_format::r8, DXGI_FORMAT_R8_UNORM },
	{ texture_format::rg8, DXGI_FORMAT_R8G8_SNORM },
	{ texture_format::rgba8, DXGI_FORMAT_R8G8B8A8_UNORM },
	{ texture_format::d24s8, DXGI_FORMAT_D24_UNORM_S8_UINT } // DXGI_FORMAT_D32_FLOAT_S8X24_UINT ?
};

static std::map<texture_filter, D3D11_FILTER> filters = {
	{ texture_filter::nearest, D3D11_FILTER_MIN_MAG_MIP_POINT },
	{ texture_filter::linear, D3D11_FILTER_MIN_MAG_MIP_LINEAR },
};

static std::map<texture_wrap, D3D11_TEXTURE_ADDRESS_MODE> wraps = {
	{ texture_wrap::clamp, D3D11_TEXTURE_ADDRESS_CLAMP },
	{ texture_wrap::repeat, D3D11_TEXTURE_ADDRESS_WRAP },
};

static std::map<texture_format, int> bytes_per_pixels = {
	{ texture_format::r8, 1 },
	{ texture_format::rg8, 2 },
	{ texture_format::rgba8, 4 },
	{ texture_format::d24s8, 4 }
};

static std::map<texture_format, bool> depth_formats = {
	{ texture_format::r8, false },
	{ texture_format::rg8, false },
	{ texture_format::rgba8, false },
	{ texture_format::d24s8, true }
};

texture_dx11::texture_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const texture_desc& desc)
	: rb::texture(desc)
	, _device(device)
	, _context(context) {
	CD3D11_TEXTURE2D_DESC texture_desc;
	texture_desc.Width = desc.size.x;
	texture_desc.Height = desc.size.y;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = formats.at(desc.format);
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.MiscFlags = 0;
	texture_desc.BindFlags = 0;
	texture_desc.CPUAccessFlags = 0;

	if (desc.is_mutable) {
		texture_desc.Usage = D3D11_USAGE_DYNAMIC;
		texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	} else {
		texture_desc.Usage = D3D11_USAGE_DEFAULT;
		texture_desc.CPUAccessFlags = 0;
	}

	if (depth_formats.at(desc.format)) {
		texture_desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	} else {
		texture_desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	}

	if (desc.is_render_target) {
		texture_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	D3D11_SUBRESOURCE_DATA subresource_data;
	ZeroMemory(&subresource_data, sizeof(subresource_data));

	if (!desc.data.empty()) {
		subresource_data.pSysMem = desc.data.data();
		subresource_data.SysMemPitch = bytes_per_pixels.at(desc.format) * desc.size.x;
		subresource_data.SysMemSlicePitch = subresource_data.SysMemPitch;
	}

	HRESULT result = device->CreateTexture2D(&texture_desc, !desc.data.empty() ? &subresource_data : nullptr, &_texture);
	assert(SUCCEEDED(result));

	if (texture_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
		CD3D11_SHADER_RESOURCE_VIEW_DESC srv_esc;
		srv_esc.Format = texture_desc.Format;
		srv_esc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_esc.Texture2D.MipLevels = 1;
		srv_esc.Texture2D.MostDetailedMip = 0;

		result = device->CreateShaderResourceView(_texture, &srv_esc, &_shader_resource_view);
		assert(SUCCEEDED(result));

		auto sampler_desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
		sampler_desc.Filter = filters.at(desc.filter);
		sampler_desc.AddressU = wraps.at(desc.wrap);
		sampler_desc.AddressV = sampler_desc.AddressU;
		sampler_desc.AddressW = sampler_desc.AddressV;
		sampler_desc.MipLODBias = 0.0f;
		sampler_desc.MaxAnisotropy = 1;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampler_desc.BorderColor[0] = sampler_desc.BorderColor[1] = sampler_desc.BorderColor[2] = sampler_desc.BorderColor[3] = 0;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

		result = device->CreateSamplerState(&sampler_desc, &_sampler_state);
		assert(SUCCEEDED(result));
	}

	if (desc.is_render_target) {
		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		render_target_view_desc.Format = texture_desc.Format;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		render_target_view_desc.Texture2D.MipSlice = 0;

		result = device->CreateRenderTargetView(_texture, &render_target_view_desc, &_render_target);
		assert(SUCCEEDED(result));
	}
}

texture_dx11::~texture_dx11() {
	if (_render_target) {
		_render_target->Release();
	}

	if (_sampler_state) {
		_sampler_state->Release();
	}

	if (_shader_resource_view) {
		_shader_resource_view->Release();
	}

	if (_texture) {
		_texture->Release();
	}
}

void texture_dx11::update(const span<const std::uint8_t>& pixels, const vec4i& rect) {
	D3D11_TEXTURE2D_DESC staging_texture_desc;
	_texture->GetDesc(&staging_texture_desc);

	staging_texture_desc.Width = rect.z;
	staging_texture_desc.Height = rect.w;
	staging_texture_desc.BindFlags = 0;
	staging_texture_desc.MiscFlags = 0;
	staging_texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	staging_texture_desc.Usage = D3D11_USAGE_STAGING;

	ID3D11Texture2D* staging_texture;
	HRESULT result = _device->CreateTexture2D(&staging_texture_desc, nullptr, &staging_texture);
	assert(SUCCEEDED(result));

	D3D11_MAPPED_SUBRESOURCE texture_memory;
	result = _context->Map(staging_texture, 0, D3D11_MAP_WRITE, 0, &texture_memory);
	assert(SUCCEEDED(result));

	const auto src = pixels.data();
	auto dst = reinterpret_cast<std::uint8_t*>(texture_memory.pData);
	UINT length = rect.z * bytes_per_pixels.at(format());
	memcpy(dst, src, static_cast<std::size_t>(length) * rect.w);

	_context->Unmap(staging_texture, 0);

	_context->CopySubresourceRegion(_texture, 0, rect.x, rect.y, 0, staging_texture, 0, nullptr);

	staging_texture->Release();
}

ID3D11Texture2D* texture_dx11::texture() const {
	return _texture;
}

ID3D11ShaderResourceView* texture_dx11::shader_resource_view() const {
	return _shader_resource_view;
}

ID3D11SamplerState* texture_dx11::sampler() const {
	return _sampler_state;
}

ID3D11RenderTargetView* texture_dx11::render_target() const {
	return _render_target;
}
