#include "graphics_device_vk.hpp"
#include "standard_shaders_vk.hpp"
#include "texture_vk.hpp"
#include "buffer_vk.hpp"

#include <rabbit/exception.hpp>
#include <rabbit/format.hpp>

#include <map>
#include <iostream>

using namespace rb;

graphics_device_vk::graphics_device_vk(const config& config, std::shared_ptr<window> window)
	: _window(window) {
	const auto ptr = &vkCreateWin32SurfaceKHR;

	auto result = volkInitialize();
	if (result != VK_SUCCESS) {
		throw exception{ "Cannot initialze vulkan!" };
	}

	const auto version = volkGetInstanceVersion();
	std::cout << format("Vulkan version {}.{}.{} initialized.", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version)) << std::endl;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = config.window.title.c_str();
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "RabBit";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = 0;
	create_info.ppEnabledExtensionNames = nullptr;
	create_info.enabledLayerCount = 0;

	result = vkCreateInstance(&create_info, nullptr, &_instance);
	if (result != VK_SUCCESS) {
		throw exception{ "Cannot create vulkan instance!" };
	}

	volkLoadInstance(_instance);
}

graphics_device_vk::~graphics_device_vk() {
	vkDestroyInstance(_instance, nullptr);
}

std::shared_ptr<texture> graphics_device_vk::make_texture(const texture_desc& desc) {
	return nullptr;
}

std::shared_ptr<buffer> graphics_device_vk::make_buffer(const buffer_desc& buffer_desc) {
	return nullptr;
}

void graphics_device_vk::clear(const color& color) {
}

void graphics_device_vk::present() {
}

void graphics_device_vk::set_blend_state(const blend_state& blend_state) {
	
}

void graphics_device_vk::set_depth_test(bool depth_test) {
}

void graphics_device_vk::set_view_matrix(const mat4f& view) {
	_view = view;
}

void graphics_device_vk::set_projection_matrix(const mat4f& projection) {
	_projection = projection;
}

void graphics_device_vk::set_world_matrix(const mat4f& world) {
	_world = world;
}

void graphics_device_vk::set_backbuffer_size(const vec2i& size) {
}

vec2i graphics_device_vk::backbuffer_size() const {
	return _window->size();
}

void graphics_device_vk::set_clip_rect(const vec4i& rect) {
}

void graphics_device_vk::set_render_target(const std::shared_ptr<texture>& texture) {
}

void graphics_device_vk::draw(topology topology, const span<const vertex>& vertices) {
	update_vertex_buffer(vertices);
}

void graphics_device_vk::draw(topology topology, std::shared_ptr<buffer> vertex_buffer) {
}

void graphics_device_vk::draw(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) {
	update_vertex_buffer(vertices);
	update_index_buffer(indices);
}

void graphics_device_vk::draw(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer) {
}

void graphics_device_vk::draw_textured(topology topology, const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) {
	update_vertex_buffer(vertices);
}

void graphics_device_vk::draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, const std::shared_ptr<texture>& texture) {
}

void graphics_device_vk::draw_textured(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const std::shared_ptr<texture>& texture) {
	update_vertex_buffer(vertices);
	update_index_buffer(indices);
}

void graphics_device_vk::draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer, const std::shared_ptr<texture>& texture) {
}

void graphics_device_vk::update_vertex_buffer(const span<const vertex>& vertices) {
}

void graphics_device_vk::update_index_buffer(const span<const std::uint32_t>& indices) {
}
