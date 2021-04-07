#pragma once 

#include <rabbit/shader.hpp>

#include <d3d11.h>

namespace rb {
    class shader_dx11 : public shader {
    public:
        shader_dx11(ID3D11Device* device, ID3D11DeviceContext* context, const shader_desc& desc);

        ~shader_dx11();

        ID3D11VertexShader* vertex_shader() const;

        ID3D11PixelShader* pixel_shader() const;

        ID3D11InputLayout* input_layout() const;
        
    private:
        ID3D11Device* _device{ nullptr };
        ID3D11DeviceContext* _context{ nullptr };
        ID3D11VertexShader* _vertex_shader{ nullptr };
        ID3D11PixelShader* _pixel_shader{ nullptr };
        ID3D11InputLayout* _input_layout{ nullptr };
    };
}
