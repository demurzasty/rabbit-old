#include "graphics_device_vulkan.hpp"
#include "buffer_vulkan.hpp"
#include "shader_vulkan.hpp"
#include "texture_cube_vulkan.hpp"
#include "texture_vulkan.hpp"

using namespace rb;

graphics_device_vulkan::graphics_device_vulkan(const config& config, window& window) {

}

graphics_device_vulkan::~graphics_device_vulkan() {

}

std::shared_ptr<texture> graphics_device_vulkan::make_texture(const texture_desc& desc) {
    return std::make_shared<texture_vulkan>(desc);
}

std::shared_ptr<texture_cube> graphics_device_vulkan::make_texture(const texture_cube_desc& texture_desc) {
    return std::make_shared<texture_cube_vulkan>(texture_desc);
}

std::shared_ptr<buffer> graphics_device_vulkan::make_buffer(const buffer_desc& buffer_desc) {
    return std::make_shared<buffer_vulkan>(buffer_desc);
}

std::shared_ptr<shader> graphics_device_vulkan::make_shader(const shader_desc& shader_desc) {
    return std::make_shared<shader_vulkan>(shader_desc);
}

std::shared_ptr<mesh> graphics_device_vulkan::make_mesh(const mesh_desc& mesh_desc) {
    return std::make_shared<mesh>(mesh_desc);
}

void graphics_device_vulkan::clear(const color& color) {

}

void graphics_device_vulkan::present() {

}

void graphics_device_vulkan::set_blend_state(const blend_state& blend_state) {

}

void graphics_device_vulkan::set_depth_test(bool depth_test) {

}

void graphics_device_vulkan::set_backbuffer_size(const vec2i& size) {

}

vec2i graphics_device_vulkan::backbuffer_size() const {
    return { 1270, 720 };
}

void graphics_device_vulkan::set_clip_rect(const vec4i& clip_rect) {

}

void graphics_device_vulkan::set_render_target(const std::shared_ptr<texture>& render_target) {

}

void graphics_device_vulkan::set_render_target(const std::shared_ptr<texture_cube>& render_target, texture_cube_face face, int level) {

}

void graphics_device_vulkan::bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) {

}

void graphics_device_vulkan::bind_texture(const std::shared_ptr<texture>& texture, std::size_t binding_index) {

}

void graphics_device_vulkan::bind_texture(const std::shared_ptr<texture_cube>& texture, std::size_t binding_index) {

}

void graphics_device_vulkan::draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader) {

}