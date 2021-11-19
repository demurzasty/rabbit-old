#pragma once 

#include <rabbit/engine/core/span.hpp>

#include <vector>

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
		static span<const std::uint32_t> geometry_nomaps_frag();
		static span<const std::uint32_t> depth_vert();
		static span<const std::uint32_t> forward_vert();
		static std::vector<std::uint32_t> forward_frag(const span<const std::string> definitions);
		static span<const std::uint32_t> skybox_vert();
		static span<const std::uint32_t> skybox_frag();
		static span<const std::uint32_t> ambient_frag();
		static span<const std::uint32_t> directional_light_frag();
		static span<const std::uint32_t> point_light_frag();
		static span<const std::uint32_t> shadowmap_vert();
		static span<const std::uint32_t> ssao_frag();
		static span<const std::uint32_t> ssao_blur_frag();
		static span<const std::uint32_t> fxaa_frag();
		static span<const std::uint32_t> blur_frag();
		static span<const std::uint32_t> sharpen_frag();
		static span<const std::uint32_t> motion_blur_frag();
		static span<const std::uint32_t> fill_vert();
		static span<const std::uint32_t> fill_frag();
		static span<const std::uint32_t> outline_frag();
		static span<const std::uint32_t> present_frag();
		static span<const std::uint32_t> light_cull_comp();
	};
}
