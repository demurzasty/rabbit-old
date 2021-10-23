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

void graphics::end() {
	_impl->end();
}

void graphics::begin_render_pass() {
	_impl->begin_render_pass();
}

void graphics::end_render_pass() {
	_impl->end_render_pass();
}

void graphics::set_camera(const transform& transform, const camera& camera) {
	_impl->set_camera(transform, camera);
}

void graphics::draw_geometry(const transform& transform, const geometry& geometry) {
	if (geometry.mesh && geometry.material) {
		_impl->draw_geometry(transform, geometry);
	}
}

void graphics::draw_skybox(const std::shared_ptr<environment>& environment) {
	if (environment) {
		_impl->draw_skybox(environment);
	}
}

void graphics::present() {
	_impl->present();
}

void graphics::flush() {
	_impl->flush();
}
