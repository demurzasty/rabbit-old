#include <rabbit/graphics.hpp>
#include <rabbit/settings.hpp>

#if RB_VULKAN
#	include "vulkan/graphics_vulkan.hpp"
#endif

using namespace rb;

std::shared_ptr<graphics_impl> graphics::_impl;

void graphics::init() {
#if RB_VULKAN
	if (settings::graphics_backend == graphics_backend::vulkan) {
		_impl = std::make_shared<graphics_vulkan>();
	}
#endif
}

void graphics::release() {
	_impl.reset();
}

std::shared_ptr<viewport> graphics::make_viewport(const viewport_desc& desc) {
	return _impl->make_viewport(desc);
}

std::shared_ptr<texture> graphics::make_texture(const texture_desc& desc) {
	return _impl->make_texture(desc);
}

std::shared_ptr<environment> graphics::make_environment(const environment_desc& desc) {
	return _impl->make_environment(desc);
}
std::shared_ptr<material> graphics::make_material(const material_desc& desc) {
	return _impl->make_material(desc);
}

std::shared_ptr<mesh> graphics::make_mesh(const mesh_desc& desc) {
	return _impl->make_mesh(desc);
}

void graphics::begin() {
	_impl->begin();
}

void graphics::set_camera(const transform& transform, const camera& camera) {
	_impl->set_camera(transform, camera);
}

void graphics::begin_geometry_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->begin_geometry_pass(viewport);
}

void graphics::draw_geometry(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry) {
	if (geometry.mesh && geometry.material) {
		_impl->draw_geometry(viewport, transform, geometry);
	}
}

void graphics::end_geometry_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->end_geometry_pass(viewport);
}

void graphics::begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, int cascade) {
	_impl->begin_shadow_pass(transform, light, directional_light, cascade);
}

void graphics::draw_shadow(const transform& transform, const geometry& geometry) {
	_impl->draw_shadow(transform, geometry);
}

void graphics::end_shadow_pass() {
	_impl->end_shadow_pass();
}

void graphics::begin_light_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->begin_light_pass(viewport);
}

void graphics::draw_ambient(const std::shared_ptr<viewport>& viewport) {
	_impl->draw_ambient(viewport);
}

void graphics::draw_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light) {
	_impl->draw_directional_light(viewport, transform, light, directional_light);
}

void graphics::draw_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light) {
	_impl->draw_point_light(viewport, transform, light, point_light);
}

void graphics::draw_skybox(const std::shared_ptr<viewport>& viewport) {
	_impl->draw_skybox(viewport);
}

void graphics::end_light_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->end_light_pass(viewport);
}

void graphics::begin_forward_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->begin_forward_pass(viewport);
}

void graphics::end_forward_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->end_forward_pass(viewport);
}

void graphics::pre_draw_ssao(const std::shared_ptr<viewport>& viewport) {
	_impl->pre_draw_ssao(viewport);
}

void graphics::begin_postprocess_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->begin_postprocess_pass(viewport);
}

void graphics::next_postprocess_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->next_postprocess_pass(viewport);
}

void graphics::draw_ssao(const std::shared_ptr<viewport>& viewport) {
	_impl->draw_ssao(viewport);
}

void graphics::draw_fxaa(const std::shared_ptr<viewport>& viewport) {
	_impl->draw_fxaa(viewport);
}

void graphics::draw_blur(const std::shared_ptr<viewport>& viewport, int strength) {
	_impl->draw_blur(viewport, std::max(strength, 0));
}

void graphics::draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength) {
	_impl->draw_sharpen(viewport, std::max(strength, 0.0f));
}

void graphics::end_postprocess_pass(const std::shared_ptr<viewport>& viewport) {
	_impl->end_postprocess_pass(viewport);
}

void graphics::begin_immediate_pass() {
	_impl->begin_immediate_pass();
}

void graphics::draw_immediate_color(const span<const vertex>& vertices, const color& color) {
	_impl->draw_immediate_color(vertices, color);
}

void graphics::draw_immediate_textured(const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) {
	_impl->draw_immediate_textured(vertices, texture);
}

void graphics::end_immediate_pass() {
	_impl->end_immediate_pass();
}

void graphics::present(const std::shared_ptr<viewport>& viewport) {
	_impl->present(viewport);
}

void graphics::end() {
	_impl->end();
}

void graphics::swap_buffers() {
	_impl->swap_buffers();
}

void graphics::flush() {
	_impl->flush();
}
