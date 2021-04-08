#include "graphics_device_dx11.hpp"
#include "texture_dx11.hpp"
#include "buffer_dx11.hpp"
#include "utils_dx11.hpp"
#include "shader_dx11.hpp"

#include <rabbit/exception.hpp>

#include <fmt/format.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "Dxgi.lib")

using namespace rb;

static std::map<topology, D3D11_PRIMITIVE_TOPOLOGY> topologies = {
	{ topology::lines, D3D10_PRIMITIVE_TOPOLOGY_LINELIST },
	{ topology::line_strip, D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP },
	{ topology::triangles, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST },
	{ topology::triangle_strip, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP }
};

static std::map<blend_function, D3D11_BLEND_OP> blend_functions = {
	{ blend_function::add, D3D11_BLEND_OP_ADD },
	{ blend_function::subtract, D3D11_BLEND_OP_SUBTRACT },
	{ blend_function::reverse_subtract, D3D11_BLEND_OP_REV_SUBTRACT },
	{ blend_function::min, D3D11_BLEND_OP_MIN },
	{ blend_function::max, D3D11_BLEND_OP_MAX }
};

static std::map<blend, D3D11_BLEND> blends = {
	{ blend::one, D3D11_BLEND_ONE },
	{ blend::zero, D3D11_BLEND_ZERO },
	{ blend::source_color, D3D11_BLEND_SRC_COLOR },
	{ blend::inverse_source_color, D3D11_BLEND_INV_SRC_COLOR },
	{ blend::source_alpha, D3D11_BLEND_SRC_ALPHA },
	{ blend::inverse_source_alpha, D3D11_BLEND_INV_SRC_ALPHA },
	{ blend::destination_color, D3D11_BLEND_DEST_COLOR },
	{ blend::inverse_destination_color, D3D11_BLEND_INV_DEST_COLOR },
	{ blend::destination_alpha, D3D11_BLEND_DEST_ALPHA },
	{ blend::inverse_destination_alpha, D3D11_BLEND_INV_DEST_ALPHA },
	{ blend::blend_factor, D3D11_BLEND_BLEND_FACTOR },
	{ blend::inverse_blend_factor, D3D11_BLEND_INV_BLEND_FACTOR },
	{ blend::source_alpha_saturation, D3D11_BLEND_SRC_ALPHA_SAT }
};

static std::map<msaa, UINT> samples = {
	{ msaa::none, 1 },
	{ msaa::x2, 2 },
	{ msaa::x4, 4 },
	{ msaa::x8, 8 },
	{ msaa::x16, 8 },
};

graphics_device_dx11::graphics_device_dx11(config& config, window& window)
	: _window(window)
	, _vsync(config.graphics.vsync) {
	ID3D11DeviceContext* context;

	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
	};

	UINT flags = 0;
#ifndef NDEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;

	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, feature_levels, 7, D3D11_SDK_VERSION, &_device, &feature_level, &context);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create d3d11 device");
	}

	result = context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&_device_context));
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot query d3d11 device context");
	}

	const auto window_size = window.size();

	IDXGIDevice2* dxgi_device;
	result = _device->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void**>(&dxgi_device));
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot query dxgi device");
	}

	result = dxgi_device->SetMaximumFrameLatency(1);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot set maximum frame latency to 1");
	}

	IDXGIAdapter* dxgi_adapter;
	result = dxgi_device->GetParent(__uuidof(IDXGIAdapter*), reinterpret_cast<void**>(&dxgi_adapter));
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot get dxgi adapter from dxgi device");
	}

	IDXGIFactory2* dxgi_factory;
	result = dxgi_adapter->GetParent(__uuidof(IDXGIFactory*), reinterpret_cast<void**>(&dxgi_factory));
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot get dxgi factory from dxgi adapter");
	}

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
	ZeroMemory(&swap_chain_desc, sizeof(swap_chain_desc));
	swap_chain_desc.Width = window_size.x;
	swap_chain_desc.Height = window_size.y;
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SampleDesc.Count = samples.at(config.window.msaa);
	swap_chain_desc.SampleDesc.Quality = 0;
	if (config.window.msaa != msaa::none) {
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
	} else {
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swap_chain_desc.Scaling = DXGI_SCALING_NONE;
	}
	swap_chain_desc.Flags = 0;

	result = dxgi_factory->CreateSwapChainForHwnd(_device, window.native_handle(), &swap_chain_desc, nullptr, nullptr, &_swap_chain);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create swap chain for window");
	}

	result = dxgi_factory->MakeWindowAssociation(window.native_handle(), DXGI_MWA_NO_WINDOW_CHANGES);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot make window association");
	}

	ID3D11Texture2D* back_buffer;
	result = _swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot get back buffer from swap chain");
	}

	result = _device->CreateRenderTargetView(back_buffer, nullptr, &_render_target);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create render target view");
	}

	back_buffer->Release();

	_depth_desc.Width = window_size.x;
	_depth_desc.Height = window_size.y;
	_depth_desc.MipLevels = 1;
	_depth_desc.ArraySize = 1;
	_depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	_depth_desc.SampleDesc.Count = samples.at(config.window.msaa);
	_depth_desc.SampleDesc.Quality = 0;
	_depth_desc.Usage = D3D11_USAGE_DEFAULT;
	_depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	_depth_desc.CPUAccessFlags = 0;
	_depth_desc.MiscFlags = 0;

	result = _device->CreateTexture2D(&_depth_desc, NULL, &_depth_stencil_texture);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create depth stencil texture");
	}

	_depth_stencil_view_desc.Format = _depth_desc.Format;
	_depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	_depth_stencil_view_desc.Texture2D.MipSlice = 0;

	result = _device->CreateDepthStencilView(_depth_stencil_texture, &_depth_stencil_view_desc, &_depth_stencil_view);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create depth stencil view");
	}

	D3D11_RASTERIZER_DESC rasterizer_desc;
	rasterizer_desc.AntialiasedLineEnable = FALSE;
	rasterizer_desc.CullMode = D3D11_CULL_NONE;
	rasterizer_desc.DepthBias = 0;
	rasterizer_desc.DepthBiasClamp = 0.0f;
	rasterizer_desc.DepthClipEnable = TRUE;
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.FrontCounterClockwise = FALSE;
	rasterizer_desc.MultisampleEnable = config.window.msaa != msaa::none ? TRUE : FALSE;
	rasterizer_desc.ScissorEnable = TRUE;
	rasterizer_desc.SlopeScaledDepthBias = 0.0f;
	result = _device->CreateRasterizerState(&rasterizer_desc, &_rasterizer_state);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create rasterizer state");
	}

	D3D11_VIEWPORT viewport = { 0 };

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(window_size.x);
	viewport.Height = static_cast<float>(window_size.y);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	_device_context->RSSetViewports(1, &viewport);

	D3D11_RECT scissor_rect;
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = window_size.x;
	scissor_rect.bottom = window_size.y;
	_device_context->RSSetScissorRects(1, &scissor_rect);

	set_blend_state(blend_state::non_premultiplied());
	set_render_target(nullptr);
}

graphics_device_dx11::~graphics_device_dx11() {
	for (auto& [hash_code, blend_state] : _blend_states) {
		safe_release(blend_state);
	}

	safe_release(_device);
	safe_release(_device_context);
	safe_release(_swap_chain);
	safe_release(_render_target);
	safe_release(_rasterizer_state);
}

std::shared_ptr<texture> graphics_device_dx11::make_texture(const texture_desc& texture_desc) {
	return std::make_shared<texture_dx11>(_device, _device_context, texture_desc);
}

std::shared_ptr<buffer> graphics_device_dx11::make_buffer(const buffer_desc& buffer_desc) {
	return std::make_shared<buffer_dx11>(_device, _device_context, buffer_desc);
}

std::shared_ptr<shader> graphics_device_dx11::make_shader(const shader_desc& shader_desc) {
	return std::make_shared<shader_dx11>(_device, _device_context, shader_desc);
}

std::shared_ptr<mesh> graphics_device_dx11::make_mesh(const mesh_desc& mesh_desc) {
	return std::make_shared<mesh>(mesh_desc);
}

void graphics_device_dx11::clear(const color& color) {
	const auto rgba = color.to_vec4<float>();

	if (_offscreen_render_target) {
		_device_context->ClearRenderTargetView(_offscreen_render_target, &rgba.x);
	} else {
		_device_context->ClearRenderTargetView(_render_target, &rgba.x);
		_device_context->ClearDepthStencilView(_depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

}

void graphics_device_dx11::present() {
	_swap_chain->Present(_vsync ? 1 : 0, 0);
}

void graphics_device_dx11::set_blend_state(const blend_state& blend_state) {
	auto& native_blend_state = _blend_states[blend_state.hash_code()];

	if (!native_blend_state) {
		D3D11_BLEND_DESC blend_desc;
		ZeroMemory(&blend_desc, sizeof(blend_desc));

		blend_desc.AlphaToCoverageEnable = FALSE;
		blend_desc.IndependentBlendEnable = TRUE;

		if (blend_state.color_source_blend != blend::one ||
			blend_state.color_destination_blend != blend::zero ||
			blend_state.alpha_source_blend != blend::one ||
			blend_state.alpha_destination_blend != blend::zero) {
			blend_desc.RenderTarget[0].BlendEnable = TRUE;
		} else {
			blend_desc.RenderTarget[0].BlendEnable = FALSE;
		}

		blend_desc.RenderTarget[0].BlendOp = blend_functions.at(blend_state.color_blend_function);
		blend_desc.RenderTarget[0].SrcBlend = blends.at(blend_state.color_source_blend);
		blend_desc.RenderTarget[0].DestBlend = blends.at(blend_state.color_destination_blend);

		blend_desc.RenderTarget[0].BlendOpAlpha = blend_functions.at(blend_state.alpha_blend_function);
		blend_desc.RenderTarget[0].SrcBlendAlpha = blends.at(blend_state.alpha_source_blend);
		blend_desc.RenderTarget[0].DestBlendAlpha = blends.at(blend_state.alpha_destination_blend);

		blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		const auto result = _device->CreateBlendState(&blend_desc, &native_blend_state);
		assert(SUCCEEDED(result));
	}

	const auto& factor = blend_state.color_factor;
	const float blend_factor[] = { factor.r / 255.0f, factor.g / 255.0f, factor.b / 255.0f, factor.a / 255.0f };

	_device_context->OMSetBlendState(native_blend_state, blend_factor, 0xffffffff);
}

void graphics_device_dx11::set_depth_test(bool depth_test) {
	_depth_test = depth_test;
}

void graphics_device_dx11::set_backbuffer_size(const vec2i& size) {
	_render_target->Release();

	_swap_chain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* buffer;
	_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
	auto result = _device->CreateRenderTargetView(buffer, NULL, &_render_target);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create render target view");
	}

	buffer->Release();

	_depth_stencil_texture->Release();

	_depth_desc.Width = size.x;
	_depth_desc.Height = size.y;

	result = _device->CreateTexture2D(&_depth_desc, NULL, &_depth_stencil_texture);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create depth stencil texture");
	}

	_depth_stencil_view->Release();

	result = _device->CreateDepthStencilView(_depth_stencil_texture, &_depth_stencil_view_desc, &_depth_stencil_view);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create depth stencil view");
	}

	D3D11_VIEWPORT viewport = { 0 };
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(size.x);
	viewport.Height = static_cast<float>(size.y);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	_device_context->RSSetViewports(1, &viewport);

	D3D11_RECT scissor_rect;
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = size.x;
	scissor_rect.bottom = size.y;
	_device_context->RSSetScissorRects(1, &scissor_rect);
}

vec2i graphics_device_dx11::backbuffer_size() const {
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
	if (SUCCEEDED(_swap_chain->GetDesc1(&swap_chain_desc))) {
		return { static_cast<int>(swap_chain_desc.Width), static_cast<int>(swap_chain_desc.Height) };
	}
	return { 0, 0 };
}


void graphics_device_dx11::set_clip_rect(const vec4i& rect) {
	D3D11_RECT scissor_rect;
	scissor_rect.left = rect.x;
	scissor_rect.top = rect.y;
	scissor_rect.right = rect.x + rect.z;
	scissor_rect.bottom = rect.y + rect.w;
	_device_context->RSSetScissorRects(1, &scissor_rect);
}

void graphics_device_dx11::set_render_target(const std::shared_ptr<texture>& texture) {
	auto native_texture = std::static_pointer_cast<texture_dx11>(texture);
	if (native_texture && native_texture->is_render_target()) {
		_offscreen_render_target = native_texture->render_target();

		const auto viewport_size = static_cast<vec2f>(native_texture->size());

		D3D11_VIEWPORT viewport = { 0 };
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = viewport_size.x;
		viewport.Height = viewport_size.y;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		_device_context->RSSetViewports(1, &viewport);
		_device_context->OMSetRenderTargets(1, &_offscreen_render_target, nullptr);
	} else {
		_offscreen_render_target = nullptr;

		const auto viewport_size = static_cast<vec2f>(_window.size());

		D3D11_VIEWPORT viewport = { 0 };
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = viewport_size.x;
		viewport.Height = viewport_size.y;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		_device_context->RSSetViewports(1, &viewport); 
		_device_context->OMSetRenderTargets(1, &_render_target, _depth_test ? _depth_stencil_view : nullptr);
	}
}

void graphics_device_dx11::bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) {
	const auto native_buffer = std::static_pointer_cast<buffer_dx11>(buffer)->buffer();

	// todo: which shader should be used? vs or ps?
	_device_context->VSSetConstantBuffers(binding_index, 1, &native_buffer);
}

void graphics_device_dx11::draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader) {
	const auto native_shader = std::static_pointer_cast<shader_dx11>(shader);

	_device_context->RSSetState(_rasterizer_state);

	const UINT stride = static_cast<UINT>(mesh->vertex_buffer()->stride());
	const UINT offset = 0;
	const auto native_buffer = std::static_pointer_cast<buffer_dx11>(mesh->vertex_buffer())->buffer();

	_device_context->IASetVertexBuffers(0, 1, &native_buffer, &stride, &offset);
	_device_context->VSSetShader(native_shader->vertex_shader(), nullptr, 0);
	_device_context->IASetInputLayout(native_shader->input_layout());
	_device_context->PSSetShader(native_shader->pixel_shader(), nullptr, 0);

	_device_context->IASetPrimitiveTopology(topologies.at(mesh->topology()));

	_device_context->Draw((UINT)mesh->vertex_buffer()->count(), 0);
}

ID3D11Device* graphics_device_dx11::device() const {
	return _device;
}

ID3D11DeviceContext1* graphics_device_dx11::device_context() const {
	return _device_context;
}