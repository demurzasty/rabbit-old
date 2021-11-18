#include "shaders_vulkan.hpp"

#include <rabbit/generated/shaders/quad.vert.spv.h>
#include <rabbit/generated/shaders/brdf.frag.spv.h>
#include <rabbit/generated/shaders/irradiance.vert.spv.h>
#include <rabbit/generated/shaders/irradiance.frag.spv.h>
#include <rabbit/generated/shaders/prefilter.vert.spv.h>
#include <rabbit/generated/shaders/prefilter.frag.spv.h>
#include <rabbit/generated/shaders/shadowmap.vert.spv.h>
#include <rabbit/generated/shaders/geometry.vert.spv.h>
#include <rabbit/generated/shaders/geometry.frag.spv.h>
#include <rabbit/generated/shaders/geometry_nomaps.frag.spv.h>
#include <rabbit/generated/shaders/depth.vert.spv.h>
#include <rabbit/generated/shaders/forward.vert.spv.h>
#include <rabbit/generated/shaders/forward.frag.glsl.h>
#include <rabbit/generated/shaders/skybox.vert.spv.h>
#include <rabbit/generated/shaders/skybox.frag.spv.h>
#include <rabbit/generated/shaders/ambient.frag.spv.h>
#include <rabbit/generated/shaders/directional_light.frag.spv.h>
#include <rabbit/generated/shaders/point_light.frag.spv.h>
#include <rabbit/generated/shaders/ssao.frag.spv.h>
#include <rabbit/generated/shaders/ssao_blur.frag.spv.h>
#include <rabbit/generated/shaders/fxaa.frag.spv.h>
#include <rabbit/generated/shaders/blur.frag.spv.h>
#include <rabbit/generated/shaders/sharpen.frag.spv.h>
#include <rabbit/generated/shaders/motion_blur.frag.spv.h>
#include <rabbit/generated/shaders/fill.vert.spv.h>
#include <rabbit/generated/shaders/fill.frag.spv.h>
#include <rabbit/generated/shaders/outline.frag.spv.h>
#include <rabbit/generated/shaders/present.frag.spv.h>
#include <rabbit/generated/shaders/light_culling.comp.spv.h>

#include <rabbit/glsl.hpp>

using namespace rb;

span<const std::uint32_t> shaders_vulkan::quad_vert() {
	return ::quad_vert;
}

span<const std::uint32_t> shaders_vulkan::brdf_frag() {
	return ::brdf_frag;
}

span<const std::uint32_t> shaders_vulkan::irradiance_vert() {
	return ::irradiance_vert;
}

span<const std::uint32_t> shaders_vulkan::irradiance_frag() {
	return ::irradiance_frag;
}

span<const std::uint32_t> shaders_vulkan::prefilter_vert() {
	return ::prefilter_vert;
}

span<const std::uint32_t> shaders_vulkan::prefilter_frag() {
	return ::prefilter_frag;
}

span<const std::uint32_t> shaders_vulkan::geometry_vert() {
	return ::geometry_vert;
}

span<const std::uint32_t> shaders_vulkan::geometry_frag() {
	return ::geometry_frag;
}

span<const std::uint32_t> shaders_vulkan::geometry_nomaps_frag() {
	return ::geometry_nomaps_frag;
}

span<const std::uint32_t> shaders_vulkan::depth_vert() {
	return ::depth_vert;
}

span<const std::uint32_t> shaders_vulkan::forward_vert() {
	return ::forward_vert;
}

std::vector<std::uint32_t> shaders_vulkan::forward_frag(const span<const std::string> definitions) {
	std::string code;
	code.resize(sizeof(::forward_frag));
	std::memcpy(&code[0], ::forward_frag, sizeof(::forward_frag));
	return glsl::compile(shader_stage::fragment, code, definitions);
}

span<const std::uint32_t> shaders_vulkan::skybox_vert() {
	return ::skybox_vert;
}

span<const std::uint32_t> shaders_vulkan::skybox_frag() {
	return ::skybox_frag;
}

span<const std::uint32_t> shaders_vulkan::ambient_frag() {
	return ::ambient_frag;
}

span<const std::uint32_t> shaders_vulkan::shadowmap_vert() {
	return ::shadowmap_vert;
}

span<const std::uint32_t> shaders_vulkan::directional_light_frag() {
	return ::directional_light_frag;
}

span<const std::uint32_t> shaders_vulkan::point_light_frag() {
	return ::point_light_frag;
}

span<const std::uint32_t> shaders_vulkan::ssao_frag() {
	return ::ssao_frag;
}

span<const std::uint32_t> shaders_vulkan::ssao_blur_frag() {
	return ::ssao_blur_frag;
}

span<const std::uint32_t> shaders_vulkan::fxaa_frag() {
	return ::fxaa_frag;
}

span<const std::uint32_t> shaders_vulkan::blur_frag() {
	return ::blur_frag;
}

span<const std::uint32_t> shaders_vulkan::sharpen_frag() {
	return ::sharpen_frag;
}

span<const std::uint32_t> shaders_vulkan::motion_blur_frag() {
	return ::motion_blur_frag;
}

span<const std::uint32_t> shaders_vulkan::fill_vert() {
	return ::fill_vert;
}

span<const std::uint32_t> shaders_vulkan::fill_frag() {
	return ::fill_frag;
}

span<const std::uint32_t> shaders_vulkan::outline_frag() {
	return ::outline_frag;
}

span<const std::uint32_t> shaders_vulkan::present_frag() {
	return ::present_frag;
}

span<const std::uint32_t> shaders_vulkan::light_cull_comp() {
	return ::light_culling_comp;
}
