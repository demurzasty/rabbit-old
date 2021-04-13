#pragma once 

#include <rabbit/graphics/texture_cube.hpp>

#include <volk.h>

namespace rb {
    class texture_cube_vulkan : public texture_cube {
    public:
		texture_cube_vulkan(const texture_cube_desc& desc);

		~texture_cube_vulkan();

		void update(texture_cube_face face, const span<const std::uint8_t>& pixels, const vec4i& rect) override;
    };
}
