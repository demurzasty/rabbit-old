#include "shader_dx11.hpp"
#include "utils_dx11.hpp"

#include <rabbit/core/exception.hpp>

#include <map>
#include <cstring>

using namespace rb;

namespace {
	std::map<vertex_attribute, const char*> semantic_names = {
		{ vertex_attribute::position, "SV_POSITION" },
		{ vertex_attribute::texcoord, "TEXCOORD" },
		{ vertex_attribute::normal, "NORMAL" }
	};

	DXGI_FORMAT get_format(const vertex_format& format) {
		if (format.type == vertex_format_type::floating_point) {
			switch (format.components) {
				case 1: return DXGI_FORMAT_R32_FLOAT;
				case 2: return DXGI_FORMAT_R32G32_FLOAT;
				case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
				case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		} else if (format.type == vertex_format_type::integer) {
			switch (format.components) {
				case 1: return DXGI_FORMAT_R32_UINT;
				case 2: return DXGI_FORMAT_R32G32_UINT;
				case 3: return DXGI_FORMAT_R32G32B32_UINT;
				case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
			}
		}

		throw make_exception("Missing vertex format");
	}
}

shader_dx11::shader_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const shader_desc& desc)
	: shader(desc)
	, _device(device)
	, _context(context) {
	auto result = _device->CreateVertexShader(desc.vertex_bytecode.data(), desc.vertex_bytecode.size_bytes(), nullptr, &_vertex_shader);
	if (FAILED(result)) {
		throw make_exception("[DX11] Cannot create vertex shader");
	}

	std::size_t index{ 0 };
	std::size_t offset{ 0 };

	D3D11_INPUT_ELEMENT_DESC vertex_desc[8];
	for (const auto& vertex_element : desc.vertex_desc) {
		vertex_desc[index].SemanticName = semantic_names.at(vertex_element.attribute);
		vertex_desc[index].SemanticIndex = 0;
		vertex_desc[index].Format = get_format(vertex_element.format);
		vertex_desc[index].InputSlot = 0;
		vertex_desc[index].AlignedByteOffset = offset;
		vertex_desc[index].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		vertex_desc[index].InstanceDataStepRate = 0;
		offset += vertex_element.format.size;
		index++;
	}

	result = _device->CreateInputLayout(vertex_desc, desc.vertex_desc.size(), desc.vertex_bytecode.data(), desc.vertex_bytecode.size_bytes(), &_input_layout);
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
