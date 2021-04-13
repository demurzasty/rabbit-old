#pragma once 

#include <rabbit/graphics/shader.hpp>

#include <volk.h>

namespace rb {
    class shader_vulkan : public shader {
    public:
        shader_vulkan(VkDevice device, const shader_desc& desc);

        ~shader_vulkan();
    
    private:
        VkShaderModule create_shader_module(const span<const std::uint8_t>& bytecode);
    
    private:
        VkDevice _device;
    };
}
