#include "buffer_dx11.hpp"
#include "utils_dx11.hpp"

#include <rabbit/exception.hpp>

#include <map>

using namespace rb;

static std::map<buffer_type, UINT> bind_flags = {
    { buffer_type::vertex, D3D11_BIND_VERTEX_BUFFER },
    { buffer_type::index, D3D11_BIND_INDEX_BUFFER },
};

buffer_dx11::buffer_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const buffer_desc& desc)
    : rb::buffer(desc)
    , _device(device)
    , _context(context) {
    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.ByteWidth = static_cast<UINT>(desc.size);
    buffer_desc.Usage = desc.is_mutable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = bind_flags.at(desc.type);
    buffer_desc.CPUAccessFlags = desc.is_mutable ? D3D11_CPU_ACCESS_WRITE : 0;

    D3D11_SUBRESOURCE_DATA buffer_data = {};
    buffer_data.pSysMem = desc.data;
    const auto result = _device->CreateBuffer(&buffer_desc, desc.data ? &buffer_data : nullptr, &_buffer);
    if (FAILED(result)) {
        throw exception{ "[DX11] Cannot create buffer" };
    }
}

buffer_dx11::~buffer_dx11() {
    safe_release(_buffer);
}

ID3D11Buffer* buffer_dx11::buffer() const {
    return _buffer;
}
