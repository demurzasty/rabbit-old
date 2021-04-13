#pragma once 

#include <rabbit/graphics/shader.hpp>

#include <volk.h>

namespace rb {
    class shader_vulkan : public shader {
    public:
        shader_vulkan(const shader_desc& desc);

        ~shader_vulkan();
    };
}
