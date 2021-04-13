#pragma once 

#include <rabbit/graphics/buffer.hpp>

#include <d3d11.h>

namespace rb {
    class buffer_dx11 : public buffer {
    public:
        buffer_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const buffer_desc& desc);

        ~buffer_dx11();

        void* map() override;

        void unmap() override;

        ID3D11Buffer* buffer() const;

    protected:
        void update(const void* data, std::size_t size) override;

    private:
        ID3D11Device* _device;
        ID3D11DeviceContext* _context;
        ID3D11Buffer* _buffer = nullptr;
    };
}
