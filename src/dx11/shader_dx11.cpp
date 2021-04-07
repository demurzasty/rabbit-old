#include "shader_dx11.hpp"
#include "utils_dx11.hpp"

#include <rabbit/exception.hpp>

#include <cstring>

using namespace rb;

shader_dx11::shader_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const shader_desc& desc)
	: _device(device)
	, _context(context) {
	auto result = _device->CreateVertexShader(desc.vertex_bytecode.data(), desc.vertex_bytecode.size_bytes(), nullptr, &_vertex_shader);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create vertex shader");
	}

	// todo: use desc.vertex_desc
	const D3D11_INPUT_ELEMENT_DESC vertex_desc[] = {
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = _device->CreateInputLayout(vertex_desc, 3, desc.vertex_bytecode.data(), desc.vertex_bytecode.size_bytes(), &_input_layout);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create input layout");
	}

	result = _device->CreatePixelShader(desc.fragment_bytecode.data(), desc.fragment_bytecode.size_bytes(), nullptr, &_pixel_shader);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create solid pixel shader");
	}
}

shader_dx11::~shader_dx11() {
	safe_release(_vertex_shader);
	safe_release(_pixel_shader);
	safe_release(_input_layout);
}

ID3D11VertexShader* shader_dx11::vertex_shader() const {
	return _vertex_shader;
}

ID3D11PixelShader* shader_dx11::pixel_shader() const {
	return _pixel_shader;
}

ID3D11InputLayout* shader_dx11::input_layout() const {
	return _input_layout;
}
