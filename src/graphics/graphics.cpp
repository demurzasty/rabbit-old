#include <rabbit/graphics/graphics.hpp>
#include <rabbit/core/settings.hpp>

#if RB_VULKAN
#	include "../drivers/vulkan/graphics_vulkan.hpp"
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

void graphics::set_camera(const mat4f& proj, const mat4f& view, const mat4f& world) {
	_impl->set_camera(proj, view, world);
}

void graphics::render(const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material) {
	_impl->render(world, mesh, material);
}

void graphics::present() {
	_impl->present();
}
