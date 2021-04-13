#pragma once 

#include <rabbit/graphics/texture.hpp>

#include <volk.h>

namespace rb {
    class texture_vulkan : public texture {
    public:
		texture_vulkan(const texture_desc& desc);

		~texture_vulkan();

		void update(const span<const std::uint8_t>& pixels, const vec4i& rect) override;
    };
}