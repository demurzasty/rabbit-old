#pragma once 

#include <rabbit/span.hpp>

namespace rb {
	class shaders_vulkan {
	public:
		static span<const std::uint32_t> quad_vert();
		static span<const std::uint32_t> brdf_frag();
		static span<const std::uint32_t> irradiance_vert();
		static span<const std::uint32_t> irradiance_frag();
		static span<const std::uint32_t> prefilter_vert();
		static span<const std::uint32_t> prefilter_frag();
		static span<const std::uint32_t> geometry_vert();
		static span<const std::uint32_t> geometry_frag();
		static span<const std::uint32_t> forward_vert();
		static span<const std::uint32_t> forward_frag();
		static span<const std::uint32_t> skybox_vert();
		static span<const std::uint32_t> skybox_frag();
		static span<const std::uint32_t> ambient_frag();
		static span<const std::uint32_t> directional_light_frag();
		static span<const std::uint32_t> point_light_frag();
		static span<const std::uint32_t> shadowmap_vert();
		static span<const std::uint32_t> ssao_frag();
		static span<const std::uint32_t> fxaa_frag();
		static span<const std::uint32_t> blur_frag();
		static span<const std::uint32_t> present_frag();
	};
}
