#include <rabbit/graphics/graphics.hpp>

#if RB_VULKAN
#include "../drivers/vulkan/graphics_vulkan.hpp"
#endif

using namespace rb;

instance graphics_impl::make_instance() {
    return _instance_registry.create();
}

void graphics_impl::destroy_instance(instance instance) {
    _instance_registry.destroy(instance);
}

basic_registry<instance>& graphics_impl::instance_registry() {
    return _instance_registry;
}

std::shared_ptr<graphics_impl> graphics::_impl;

void graphics::setup() {
#if RB_VULKAN
    _impl = std::make_shared<graphics_vulkan>();
#endif
}

void graphics::release() {
    _impl.reset();
}

void graphics::set_camera_projection_matrix(const mat4f& projection_matrix) {
    _impl->set_camera_projection_matrix(projection_matrix);
}

void graphics::set_camera_view_matrix(const mat4f& view_matrix) {
    _impl->set_camera_view_matrix(view_matrix);
}

void graphics::set_camera_world_matrix(const mat4f& world_matrix) {
    _impl->set_camera_world_matrix(world_matrix);
}

instance graphics::make_instance() {
    return _impl->make_instance();
}

void graphics::destroy_instance(instance instance) {
    _impl->destroy_instance(instance);
}

void graphics::set_instance_mesh(instance instance, const std::shared_ptr<mesh>& mesh) {
    _impl->set_instance_mesh(instance, mesh);
}

void graphics::set_instance_material(instance instance, const std::shared_ptr<material>& material) {
    _impl->set_instance_material(instance, material);
}

void graphics::set_instance_world_matrix(instance instance, const mat4f& world_matrix) {
    _impl->set_instance_world_matrix(instance, world_matrix);
}

void graphics::draw() {
    _impl->draw();
}

void graphics::swap_buffers() {
    _impl->swap_buffers();
}
