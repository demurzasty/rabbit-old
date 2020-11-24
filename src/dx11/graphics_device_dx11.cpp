#include "graphics_device_dx11.hpp"
#include "standard_shaders_dx11.hpp"
#include "texture_dx11.hpp"
#include "buffer_dx11.hpp"
#include "utils_dx11.hpp"

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

graphics_device_dx11::graphics_device_dx11(const config& config, std::shared_ptr<window> window)
	: _window(window)
	, _vsync(config.graphics.vsync) {
	ID3D11DeviceContext* context;

	D3D_FEATURE_LEVEL feature_levels[] = {
		D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
	};

	UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;

	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, feature_levels, 7, D3D11_SDK_VERSION, &_device, &feature_level, &context);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create d3d11 device" };
	}

	result = context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&_device_context));
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot query d3d11 device context" };
	}

	const auto window_size = window->size();

	IDXGIDevice2* dxgi_device;
	result = _device->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void**>(&dxgi_device));
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot query dxgi device" };
	}

	result = dxgi_device->SetMaximumFrameLatency(1);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot set maximum frame latency to 1" };
	}

	IDXGIAdapter* dxgi_adapter;
	result = dxgi_device->GetParent(__uuidof(IDXGIAdapter*), reinterpret_cast<void**>(&dxgi_adapter));
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot get dxgi adapter from dxgi device" };
	}

	IDXGIFactory2* dxgi_factory;
	result = dxgi_adapter->GetParent(__uuidof(IDXGIFactory*), reinterpret_cast<void**>(&dxgi_factory));
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot get dxgi factory from dxgi adapter" };
	}

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;
	ZeroMemory(&swap_chain_desc, sizeof(swap_chain_desc));
	swap_chain_desc.Width = window_size.x;
	swap_chain_desc.Height = window_size.y;
	swap_chain_desc.BufferCount = 2;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swap_chain_desc.Scaling = DXGI_SCALING_NONE;
	swap_chain_desc.Flags = 0;

	result = dxgi_factory->CreateSwapChainForHwnd(_device, window->native_handle(), &swap_chain_desc, nullptr, nullptr, &_swap_chain);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create swap chain for window" };
	}

	result = dxgi_factory->MakeWindowAssociation(window->native_handle(), DXGI_MWA_NO_WINDOW_CHANGES);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot make window association" };
	}

	ID3D11Texture2D* back_buffer;
	result = _swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot get back buffer from swap chain" };
	}

	result = _device->CreateRenderTargetView(back_buffer, nullptr, &_render_target);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create render target view" };
	}

	back_buffer->Release();

	D3D11_TEXTURE2D_DESC depth_desc = {};
	depth_desc.Width = window_size.x;
	depth_desc.Height = window_size.y;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_desc.SampleDesc.Count = 1;
	depth_desc.SampleDesc.Quality = 0;
	depth_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_desc.CPUAccessFlags = 0;
	depth_desc.MiscFlags = 0;

	result = _device->CreateTexture2D(&depth_desc, NULL, &_depth_stencil_texture);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create depth stencil texture" };
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};

	depth_stencil_view_desc.Format = depth_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;

	result = _device->CreateDepthStencilView(_depth_stencil_texture, &depth_stencil_view_desc, &_depth_stencil_view);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create depth stencil view" };
	}

	const auto vertex_shader = standard_shaders_dx11::vertex_shader();
	result = _device->CreateVertexShader(vertex_shader.data(), vertex_shader.size_bytes(), nullptr, &_vertex_shader);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create vertex shader" };
	}

	const D3D11_INPUT_ELEMENT_DESC vertex_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = _device->CreateInputLayout(vertex_desc, 3, vertex_shader.data(), vertex_shader.size_bytes(), &_input_layout);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create input layout" };
	}

	const auto solid_pixel_shader = standard_shaders_dx11::solid_pixel_shader();
	result = _device->CreatePixelShader(solid_pixel_shader.data(), solid_pixel_shader.size_bytes(), nullptr, &_color_pixel_shader);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create solid pixel shader" };
	}

	const auto texture_pixel_shader = standard_shaders_dx11::texture_pixel_shader();
	result = _device->CreatePixelShader(texture_pixel_shader.data(), texture_pixel_shader.size_bytes(), nullptr, &_texture_pixel_shader);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create texture pixel shader" };
	}

	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(D3D11_BUFFER_DESC));
	constant_buffer_desc.ByteWidth = sizeof(vertex_shader_data);
	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	result = _device->CreateBuffer(&constant_buffer_desc, nullptr, &_vertex_constant_buffer);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create constant buffer" };
	}

	D3D11_RASTERIZER_DESC rasterizer_desc;
	rasterizer_desc.AntialiasedLineEnable = FALSE;
	rasterizer_desc.CullMode = D3D11_CULL_NONE;
	rasterizer_desc.DepthBias = 0;
	rasterizer_desc.DepthBiasClamp = 0.0f;
	rasterizer_desc.DepthClipEnable = TRUE;
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.FrontCounterClockwise = FALSE;
	rasterizer_desc.MultisampleEnable = FALSE;
	rasterizer_desc.ScissorEnable = TRUE;
	rasterizer_desc.SlopeScaledDepthBias = 0.0f;
	result = _device->CreateRasterizerState(&rasterizer_desc, &_rasterizer_state);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create rasterizer state" };
	}

	D3D11_VIEWPORT viewport = { 0 };

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(window_size.x);
	viewport.Height = static_cast<float>(window_size.y);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	_device_context->RSSetViewports(1, &viewport);

	_device_context->IASetInputLayout(_input_layout);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);

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
	safe_release(_vertex_buffer);
	safe_release(_vertex_constant_buffer);
	safe_release(_vertex_shader);
	safe_release(_color_pixel_shader);
	safe_release(_texture_pixel_shader);
	safe_release(_input_layout);
	safe_release(_rasterizer_state);
}

std::shared_ptr<texture> graphics_device_dx11::make_texture(const texture_desc& texture_desc) {
	return std::make_shared<texture_dx11>(_device, _device_context, texture_desc);
}

std::shared_ptr<buffer> graphics_device_dx11::make_buffer(const buffer_desc& buffer_desc) {
	return std::make_shared<buffer_dx11>(_device, _device_context, buffer_desc);
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

void graphics_device_dx11::set_view_matrix(const mat4f& view) {
	_vertex_shader_data.view = view;
}

void graphics_device_dx11::set_projection_matrix(const mat4f& projection) {
	_vertex_shader_data.projection = projection;
}

void graphics_device_dx11::set_world_matrix(const mat4f& world) {
	_vertex_shader_data.world = world;
}

void graphics_device_dx11::set_backbuffer_size(const vec2i& size) {
	_render_target->Release();

	_swap_chain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* buffer;
	_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buffer);
	auto result = _device->CreateRenderTargetView(buffer, NULL, &_render_target);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create render target view" };
	}

	buffer->Release();

	_depth_stencil_texture->Release();

	D3D11_TEXTURE2D_DESC depth_desc = {};
	depth_desc.Width = size.x;
	depth_desc.Height = size.y;
	depth_desc.MipLevels = 1;
	depth_desc.ArraySize = 1;
	depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_desc.SampleDesc.Count = 1;
	depth_desc.SampleDesc.Quality = 0;
	depth_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_desc.CPUAccessFlags = 0;
	depth_desc.MiscFlags = 0;

	result = _device->CreateTexture2D(&depth_desc, NULL, &_depth_stencil_texture);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create depth stencil texture" };
	}

	_depth_stencil_view->Release();

	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};

	depth_stencil_view_desc.Format = depth_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;

	result = _device->CreateDepthStencilView(_depth_stencil_texture, &depth_stencil_view_desc, &_depth_stencil_view);
	if (FAILED(result)) {
		throw exception{ "[DX11] Cannot create depth stencil view" };
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

		const auto viewport_size = static_cast<vec2f>(_window->size());

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

void graphics_device_dx11::draw(topology topology, const span<const vertex>& vertices) {
	_device_context->RSSetState(_rasterizer_state);

	update_vertex_buffer(vertices);

	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);

	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_color_pixel_shader, nullptr, 0);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->Draw((UINT)vertices.size(), 0);
}

void graphics_device_dx11::draw(topology topology, std::shared_ptr<buffer> vertex_buffer) {
	if (!vertex_buffer) {
		throw exception{ "[DX11] Buffer need to be provided" };
	}

	if (vertex_buffer->type() != buffer_type::vertex) {
		throw exception{ "[DX11] Buffer need to be vertex type" };
	}

	_device_context->RSSetState(_rasterizer_state);
	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);
	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);

	const UINT stride = static_cast<UINT>(vertex_buffer->stride());
	const UINT offset = 0;
	auto native_buffer = std::static_pointer_cast<buffer_dx11>(vertex_buffer)->buffer();

	_device_context->IASetVertexBuffers(0, 1, &native_buffer, &stride, &offset);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_color_pixel_shader, nullptr, 0);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->Draw(static_cast<UINT>(vertex_buffer->count()), 0);
}

void graphics_device_dx11::draw(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer) {
	if (!vertex_buffer) {
		throw exception{ "[DX11] Vertex buffer need to be provided" };
	}

	if (vertex_buffer->type() != buffer_type::vertex) {
		throw exception{ "[DX11] Vertex buffer need to be vertex type" };
	}	
	
	if (!index_buffer) {
		throw exception{ "[DX11] Index buffer need to be provided" };
	}

	if (index_buffer->type() != buffer_type::index) {
		throw exception{ "[DX11] Index buffer to be index type" };
	}

	_device_context->RSSetState(_rasterizer_state);
	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);
	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);

	const UINT stride = static_cast<UINT>(vertex_buffer->stride());
	const UINT offset = 0;
	const auto native_buffer = std::static_pointer_cast<buffer_dx11>(vertex_buffer)->buffer();

	_device_context->IASetVertexBuffers(0, 1, &native_buffer, &stride, &offset);
	_device_context->IASetIndexBuffer(std::static_pointer_cast<buffer_dx11>(index_buffer)->buffer(), DXGI_FORMAT_R32_UINT, 0);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_color_pixel_shader, nullptr, 0);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->DrawIndexed(static_cast<UINT>(index_buffer->count()), 0, 0);
}

void graphics_device_dx11::draw(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) {
	_device_context->RSSetState(_rasterizer_state);

	update_vertex_buffer(vertices);
	update_index_buffer(indices);

	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);

	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_color_pixel_shader, nullptr, 0);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void graphics_device_dx11::draw_textured(topology topology, const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) {
	_device_context->RSSetState(_rasterizer_state);

	update_vertex_buffer(vertices);

	auto native_texture = std::static_pointer_cast<texture_dx11>(texture);

	auto shader_resource_view = native_texture->shader_resource_view();
	auto sampler_state = native_texture->sampler();

	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);

	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_texture_pixel_shader, nullptr, 0);
	_device_context->PSSetShaderResources(0, 1, &shader_resource_view);
	_device_context->PSSetSamplers(0, 1, &sampler_state);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->Draw(static_cast<UINT>(vertices.size()), 0);
}

void graphics_device_dx11::draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, const std::shared_ptr<texture>& texture) {
	if (!vertex_buffer) {
		throw exception{ "[DX11] Vertex buffer need to be provided" };
	}

	if (vertex_buffer->type() != buffer_type::vertex) {
		throw exception{ "[DX11] Vertex buffer need to be vertex type" };
	}

	const auto native_texture = std::static_pointer_cast<texture_dx11>(texture);

	ID3D11ShaderResourceView* shader_resource_view = native_texture->shader_resource_view();
	ID3D11SamplerState* sampler_state = native_texture->sampler();

	_device_context->RSSetState(_rasterizer_state);
	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);
	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);

	const UINT stride = static_cast<UINT>(vertex_buffer->stride());
	const UINT offset = 0;
	const auto native_buffer = std::static_pointer_cast<buffer_dx11>(vertex_buffer)->buffer();

	_device_context->IASetVertexBuffers(0, 1, &native_buffer, &stride, &offset);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_texture_pixel_shader, nullptr, 0);
	_device_context->PSSetShaderResources(0, 1, &shader_resource_view);
	_device_context->PSSetSamplers(0, 1, &sampler_state);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->Draw(static_cast<UINT>(vertex_buffer->count()), 0);
}

void graphics_device_dx11::draw_textured(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const std::shared_ptr<texture>& texture) {
	_device_context->RSSetState(_rasterizer_state);

	update_vertex_buffer(vertices);
	update_index_buffer(indices);

	const auto native_texture = std::static_pointer_cast<texture_dx11>(texture);

	ID3D11ShaderResourceView* shader_resource_view = native_texture->shader_resource_view();
	ID3D11SamplerState* sampler_state = native_texture->sampler();

	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);

	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_texture_pixel_shader, nullptr, 0);
	_device_context->PSSetShaderResources(0, 1, &shader_resource_view);
	_device_context->PSSetSamplers(0, 1, &sampler_state);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void graphics_device_dx11::draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer, const std::shared_ptr<texture>& texture) {
	if (!vertex_buffer) {
		throw exception{ "[DX11] Vertex buffer need to be provided" };
	}

	if (vertex_buffer->type() != buffer_type::vertex) {
		throw exception{ "[DX11] Vertex buffer need to be vertex type" };
	}

	if (!index_buffer) {
		throw exception{ "[DX11] Index buffer need to be provided" };
	}

	if (index_buffer->type() != buffer_type::index) {
		throw exception{ "[DX11] Index buffer to be index type" };
	}

	const auto native_texture = std::static_pointer_cast<texture_dx11>(texture);

	ID3D11ShaderResourceView* shader_resource_view = native_texture->shader_resource_view();
	ID3D11SamplerState* sampler_state = native_texture->sampler();

	_device_context->RSSetState(_rasterizer_state);
	_device_context->UpdateSubresource(_vertex_constant_buffer, 0, nullptr, &_vertex_shader_data, 0, 0);
	_device_context->VSSetConstantBuffers(0, 1, &_vertex_constant_buffer);

	const UINT stride = static_cast<UINT>(vertex_buffer->stride());
	const UINT offset = 0;
	const auto native_buffer = std::static_pointer_cast<buffer_dx11>(vertex_buffer)->buffer();

	_device_context->IASetVertexBuffers(0, 1, &native_buffer, &stride, &offset);
	_device_context->IASetIndexBuffer(std::static_pointer_cast<buffer_dx11>(index_buffer)->buffer(), DXGI_FORMAT_R32_UINT, 0);
	_device_context->VSSetShader(_vertex_shader, nullptr, 0);
	_device_context->IASetInputLayout(_input_layout);
	_device_context->PSSetShader(_texture_pixel_shader, nullptr, 0);
	_device_context->PSSetShaderResources(0, 1, &shader_resource_view);
	_device_context->PSSetSamplers(0, 1, &sampler_state);

	_device_context->IASetPrimitiveTopology(topologies.at(topology));
	_device_context->DrawIndexed(static_cast<UINT>(index_buffer->count()), 0, 0);
}

ID3D11Device* graphics_device_dx11::device() const {
	return _device;
}

ID3D11DeviceContext1* graphics_device_dx11::device_context() const {
	return _device_context;
}

void graphics_device_dx11::update_vertex_buffer(const span<const vertex>& vertices) {
	D3D11_BUFFER_DESC desc;
	if (_vertex_buffer) {
		_vertex_buffer->GetDesc(&desc);
	} else {
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	}

	if (_vertex_buffer && vertices.size_bytes() <= desc.ByteWidth) {
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		const HRESULT result = _device_context->Map(_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
		if (FAILED(result)) {
			throw exception{ "[DX11] Cannot map vertex buffer" };
		}

		memcpy(mapped_resource.pData, vertices.data(), vertices.size_bytes());
		_device_context->Unmap(_vertex_buffer, 0);
	} else {
		if (_vertex_buffer) {
			_vertex_buffer->Release();
		}

		desc.ByteWidth = static_cast<UINT>(vertices.size_bytes());
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA vertex_buffer_data = {};
		vertex_buffer_data.pSysMem = vertices.data();

		const HRESULT result = _device->CreateBuffer(&desc, &vertex_buffer_data, &_vertex_buffer);
		if (FAILED(result)) {
			throw exception{ "[DX11] Cannot create vertex buffer" };
		}
	}

	const UINT stride = sizeof(vertex);
	const UINT offset = 0;

	_device_context->IASetVertexBuffers(0, 1, &_vertex_buffer, &stride, &offset);
}

void graphics_device_dx11::update_index_buffer(const span<const std::uint32_t>& indices) {
	D3D11_BUFFER_DESC desc;
	if (_index_buffer) {
		_index_buffer->GetDesc(&desc);
	} else {
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	}

	if (_index_buffer && indices.size_bytes() <= desc.ByteWidth) {
		D3D11_MAPPED_SUBRESOURCE mapped_resource;

		const auto result = _device_context->Map(_index_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
		if (FAILED(result)) {
			throw exception{ "[DX11] Cannot map index buffer" };
		}

		memcpy(mapped_resource.pData, indices.data(), indices.size_bytes());
		_device_context->Unmap(_index_buffer, 0);
	} else {
		if (_index_buffer) {
			_index_buffer->Release();
		}

		desc.ByteWidth = static_cast<UINT>(indices.size_bytes());
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA index_buffer_data;
		ZeroMemory(&index_buffer_data, sizeof(D3D11_SUBRESOURCE_DATA));
		index_buffer_data.pSysMem = indices.data();
		index_buffer_data.SysMemPitch = 0;
		index_buffer_data.SysMemSlicePitch = 0;

		const auto result = _device->CreateBuffer(&desc, &index_buffer_data, &_index_buffer);
		if (FAILED(result)) {
			throw exception{ "[DX11] Cannot create index buffer" };
		}
	}

	_device_context->IASetIndexBuffer(_index_buffer, DXGI_FORMAT_R32_UINT, 0);
}
