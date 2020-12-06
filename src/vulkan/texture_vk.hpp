#pragma once 

#include <rabbit/texture.hpp>

namespace rb {
    class texture_vk : public texture {
    public:
		texture_vk(const texture_desc& desc);

		~texture_vk();

		void update(const span<const std::uint8_t>& pixels, const vec4i& rect) override;
    };
}