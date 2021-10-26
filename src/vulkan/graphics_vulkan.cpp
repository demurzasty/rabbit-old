#include "graphics_vulkan.hpp"
#include "texture_vulkan.hpp"
#include "environment_vulkan.hpp"
#include "material_vulkan.hpp"
#include "mesh_vulkan.hpp"
#include "shaders_vulkan.hpp"
#include "utils_vulkan.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace rb;

// TODO: Refactor redundant code.
// TODO: Reuse quad vert shader module where possible.
// TODO: Use secondary command buffer to bake shadow map.

namespace {
#if _DEBUG
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    VKAPI_ATTR
    VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {
        std::fprintf(stderr, "%s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }
}


graphics_vulkan::graphics_vulkan() {
    _initialize_volk();
    _create_instance();
    _choose_physical_device();
    _create_surface();
    _create_device();
    _create_allocator();
    _query_surface();
    _create_swapchain();
    _create_command_pool();
    _create_synchronization_objects();
    _create_quad();
    _generate_brdf_image();
    _create_camera_buffer();
    _create_skybox();
    _create_irradiance_pipeline();
    _create_prefilter_pipeline();
    _create_shadow_map();
    _create_forward_pipeline();
    _create_gbuffer(); // deferred
    _create_ambient_pipeline(); // deferred
    _create_directional_light_pipeline(); // deferred
    _create_skybox_pipeline();
    _create_command_buffers();
}

graphics_vulkan::~graphics_vulkan() {
    vkQueueWaitIdle(_graphics_queue);
    vkQueueWaitIdle(_present_queue);
    vkDeviceWaitIdle(_device);

    vkDestroyPipeline(_device, _skybox_pipeline, nullptr);
    vkDestroyShaderModule(_device, _skybox_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, _skybox_shader_modules[0], nullptr);
    vkDestroyPipelineLayout(_device, _skybox_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _gbuffer_pipeline, nullptr);
    vkDestroyShaderModule(_device, _gbuffer_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, _gbuffer_shader_modules[0], nullptr);
    vkDestroyPipelineLayout(_device, _gbuffer_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _gbuffer_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(_device, _gbuffer_descriptor_pool, nullptr);
    vkDestroyFramebuffer(_device, _gbuffer_framebuffer, nullptr);
    vkDestroyRenderPass(_device, _gbuffer_render_pass, nullptr);
    vkDestroySampler(_device, _gbuffer_sampler, nullptr);

    for (auto i = 0u; i < 3; ++i) {
        vkDestroyImageView(_device, _gbuffer_views[i], nullptr);
        vmaDestroyImage(_allocator, _gbuffer[i], _gbuffer_allocations[i]);
    }

    vkDestroyPipeline(_device, _directional_light_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _directional_light_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _ambient_pipeline, nullptr);
    vkDestroyShaderModule(_device, _ambient_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, _ambient_shader_modules[0], nullptr);
    vkDestroyPipelineLayout(_device, _ambient_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _forward_pipeline, nullptr);
    vkDestroyShaderModule(_device, _forward_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, _forward_shader_modules[0], nullptr);
    vkDestroyPipelineLayout(_device, _forward_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _forward_descriptor_set_layout[2], nullptr);
    vkDestroyDescriptorSetLayout(_device, _forward_descriptor_set_layout[1], nullptr);
    vkDestroyDescriptorSetLayout(_device, _forward_descriptor_set_layout[0], nullptr);
    vkDestroyDescriptorPool(_device, _forward_descriptor_pool, nullptr);

    vkDestroyPipeline(_device, _shadow_pipeline, nullptr);
    vkDestroyShaderModule(_device, _shadow_shader_module, nullptr);
    vkDestroyPipelineLayout(_device, _shadow_pipeline_layout, nullptr);
    vkDestroyFramebuffer(_device, _shadow_framebuffer, nullptr);
    vkDestroyRenderPass(_device, _shadow_render_pass, nullptr);
    vkDestroySampler(_device, _shadow_sampler, nullptr);
    vkDestroyImageView(_device, _shadow_image_view, nullptr);
    vmaDestroyImage(_allocator, _shadow_image, _shadow_allocation);

    vkDestroyPipeline(_device, _prefilter_pipeline, nullptr);
    vkDestroyRenderPass(_device, _prefilter_render_pass, nullptr);
    vkDestroyShaderModule(_device, _prefilter_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, _prefilter_shader_modules[0], nullptr);
    vkDestroyPipelineLayout(_device, _prefilter_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _prefilter_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(_device, _prefilter_descriptor_pool, nullptr);

    vkDestroyPipeline(_device, _irradiance_pipeline, nullptr);
    vkDestroyRenderPass(_device, _irradiance_render_pass, nullptr);
    vkDestroyShaderModule(_device, _irradiance_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, _irradiance_shader_modules[0], nullptr);
    vkDestroyPipelineLayout(_device, _irradiance_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _irradiance_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(_device, _irradiance_descriptor_pool, nullptr);

    vmaDestroyBuffer(_allocator, _skybox_vertex_buffer, _skybox_vertex_allocation);
    vmaDestroyBuffer(_allocator, _skybox_index_buffer, _skybox_index_allocation);

    vmaDestroyBuffer(_allocator, _light_buffer, _light_allocation);
    vmaDestroyBuffer(_allocator, _camera_buffer, _camera_allocation);

    vkDestroySampler(_device, _brdf_sampler, nullptr);
    vkDestroyImageView(_device, _brdf_image_view, nullptr);
    vmaDestroyImage(_allocator, _brdf_image, _brdf_allocation);

    vmaDestroyBuffer(_allocator, _quad_vertex_buffer, _quad_vertex_allocation);
    vmaDestroyBuffer(_allocator, _quad_index_buffer, _quad_index_allocation);

    vkDestroySemaphore(_device, _present_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_semaphore, nullptr);
    vkDestroyCommandPool(_device, _command_pool, nullptr);
    vkDestroyRenderPass(_device, _second_render_pass, nullptr);
    vkDestroyRenderPass(_device, _render_pass, nullptr);

    for (auto framebuffer : _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }

    for (auto image_view : _image_views) {
        vkDestroyImageView(_device, image_view, nullptr);
    }

    vkDestroyImageView(_device, _depth_image_view, nullptr);
    vmaDestroyImage(_allocator, _depth_image, _depth_image_allocation);

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

std::shared_ptr<texture> graphics_vulkan::make_texture(const texture_desc& desc) {
    return std::make_shared<texture_vulkan>(_device, _physical_device_properties, _graphics_queue, _command_pool, _allocator, desc);
}

std::shared_ptr<environment> graphics_vulkan::make_environment(const environment_desc& desc) {
    const auto environment = std::make_shared<environment_vulkan>(_device, _graphics_queue, _command_pool, _allocator, _forward_descriptor_set_layout[2], desc);
    _bake_irradiance(environment);
    _bake_prefilter(environment);
    return environment;
}

std::shared_ptr<material> graphics_vulkan::make_material(const material_desc& desc) {
	return std::make_shared<material_vulkan>(_device, _allocator, _forward_descriptor_set_layout[1], desc);
}

std::shared_ptr<mesh> graphics_vulkan::make_mesh(const mesh_desc& desc) {
    return std::make_shared<mesh_vulkan>(_device, _allocator, desc);
}

void graphics_vulkan::begin() {
    _command_begin();

    _first_render_pass = true;
}

void graphics_vulkan::set_camera(const transform& transform, const camera& camera) {
    _environment = std::static_pointer_cast<environment_vulkan>(camera.environment);

    const auto aspect = static_cast<float>(_swapchain_extent.width) / _swapchain_extent.height;
    _camera_data.projection = mat4f::perspective(deg2rad(camera.field_of_view), aspect, camera.z_near, camera.z_far);
    _camera_data.view = invert(mat4f::translation(transform.position) * mat4f::rotation(transform.rotation));
    _camera_data.inv_proj_view = invert(_camera_data.projection * _camera_data.view);
    _camera_data.camera_position = transform.position;

    vkCmdUpdateBuffer(_command_buffers[_command_index], _camera_buffer, 0, sizeof(camera_data), &_camera_data);
}

void graphics_vulkan::begin_geometry_pass() {
    VkClearValue clear_values[4];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[3].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _gbuffer_render_pass;
    render_pass_begin_info.framebuffer = _gbuffer_framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain_extent;
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _gbuffer_pipeline);
}

void graphics_vulkan::draw_geometry(const transform& transform, const geometry& geometry) {
    const auto native_material = std::static_pointer_cast<material_vulkan>(geometry.material);
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(geometry.mesh);

    VkDescriptorSet descriptor_sets[]{
        _forward_descriptor_set,
        native_material->descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _gbuffer_pipeline_layout, 0, 2, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    VkBuffer buffer{ native_mesh->vertex_buffer() };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], native_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

    local_data local_data;
    local_data.world = mat4f::translation(transform.position) *
        mat4f::rotation(transform.rotation) *
        mat4f::scaling(transform.scaling);

    vkCmdPushConstants(_command_buffers[_command_index], _gbuffer_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(local_data), &local_data);

    vkCmdDrawIndexed(_command_buffers[_command_index], static_cast<std::uint32_t>(native_mesh->indices().size()), 1, 0, 0, 0);
}

void graphics_vulkan::end_geometry_pass() {
    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light) {
    const auto dir = normalize(transform_normal(mat4f::rotation(transform.rotation), vec3f{ 0.0f, 0.0f, 1.0f }));
    const auto depth_projection = mat4f::orthographic(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    const auto depth_view = mat4f::look_at(_camera_data.camera_position - dir * 10.0f, _camera_data.camera_position, { 0.0f, 1.0f, 0.0f });
    _light_proj_view = depth_projection * depth_view;

    VkClearValue clear_values[1];
    clear_values[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _shadow_render_pass;
    render_pass_begin_info.framebuffer = _shadow_framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { graphics_limits::shadow_map_size, graphics_limits::shadow_map_size };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _shadow_pipeline);
}

void graphics_vulkan::draw_shadow(const transform& transform, const geometry& geometry) {
    const auto native_material = std::static_pointer_cast<material_vulkan>(geometry.material);
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(geometry.mesh);

    VkDeviceSize offset{ 0 };
    VkBuffer buffer{ native_mesh->vertex_buffer() };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], native_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

    const auto world = mat4f::translation(transform.position) *
        mat4f::rotation(transform.rotation) *
        mat4f::scaling(transform.scaling);

    shadow_data shadow_data;
    shadow_data.proj_view_world = _light_proj_view * world;
    vkCmdPushConstants(_command_buffers[_command_index], _shadow_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shadow_data), &shadow_data);

    vkCmdDrawIndexed(_command_buffers[_command_index], static_cast<std::uint32_t>(native_mesh->indices().size()), 1, 0, 0, 0);
}

void graphics_vulkan::end_shadow_pass() {
    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_render_pass() {
    // No need to clear depth buffer because we will resue it from gbuffer
    VkClearValue clear_values[1];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _first_render_pass ? _render_pass : _second_render_pass;
    render_pass_begin_info.framebuffer = _framebuffers[_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain_extent;
    render_pass_begin_info.clearValueCount = 1; // sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    _first_render_pass = false;
}

void graphics_vulkan::draw_ambient() {
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _ambient_pipeline);

    VkDescriptorSet descriptor_sets[]{
        _forward_descriptor_set,
        _gbuffer_descriptor_set,
        _environment->descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _ambient_pipeline_layout, 0, 3, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_directional_light(const transform& transform, const light& light, const directional_light& directional_light) {
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _directional_light_pipeline);

    const auto dir = normalize(transform_normal(mat4f::rotation(transform.rotation), vec3f{ 0.0f, 0.0f, 1.0f }));
    const auto depth_projection = mat4f::orthographic(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    const auto depth_view = mat4f::look_at(_camera_data.camera_position - dir * 10.0f, _camera_data.camera_position, { 0.0f, 1.0f, 0.0f });
    const auto proj_view = depth_projection * depth_view;

    VkDescriptorSet descriptor_sets[]{
        _forward_descriptor_set,
        _gbuffer_descriptor_set,
        _environment->descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _directional_light_pipeline_layout, 0, 3, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    directional_light_data directional_light_data;
    directional_light_data.light_dir = dir;
    directional_light_data.light_color = { light.color.r / 255.0f, light.color.g / 255.0f, light.color.b / 255.0f };
    directional_light_data.light_proj_view = _light_proj_view;

    vkCmdPushConstants(_command_buffers[_command_index], _directional_light_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(directional_light_data), &directional_light_data);

    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_skybox() {
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox_pipeline);

    VkDescriptorSet descriptor_sets[]{
        _forward_descriptor_set,
        _environment->descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox_pipeline_layout, 0, 2, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_skybox_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _skybox_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(_command_buffers[_command_index], 36, 1, 0, 0, 0);
}

void graphics_vulkan::end_render_pass() {
    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::end() {
    _command_end();
}

void graphics_vulkan::present() {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &_present_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 0;
    submit_info.pCommandBuffers = nullptr;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &_render_semaphore;

    RB_VK(vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE), "Failed to queue submit");

    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;

    // We should wait for rendering execution.
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &_render_semaphore;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;

    present_info.pImageIndices = &_image_index;

    present_info.pResults = nullptr;

    RB_VK(vkQueuePresentKHR(_graphics_queue, &present_info), "Failed to queue present");
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, nullptr, &_image_index), "Failed to reset acquire next swapchain image");
}

void graphics_vulkan::flush() {
    for (auto& fence : _fences) {
        vkWaitForFences(_device, 1, &fence, VK_TRUE, 1000000000);
        vkDestroyFence(_device, fence, nullptr);
    }
    vkFreeCommandBuffers(_device, _command_pool, 3, _command_buffers);

    _environment.reset();
}

void graphics_vulkan::_initialize_volk() {
    RB_VK(volkInitialize(), "Cannot initialize volk library.");
}

void graphics_vulkan::_create_instance() {
    const auto [major, minor, patch] = settings::app_version;

    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = settings::window_title.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(major, minor, patch);
    app_info.pEngineName = "RabBit";
    app_info.engineVersion = VK_MAKE_VERSION(RB_VERSION_MAJOR, RB_VERSION_MINOR, RB_VERSION_PATCH);
    app_info.apiVersion = VK_API_VERSION_1_0;

    const char* enabled_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if RB_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
    };

#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_info;
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.pNext = nullptr;
    debug_info.flags = 0;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = &debug_callback;
    debug_info.pUserData = nullptr;
#endif

    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef _DEBUG
    instance_info.pNext = &debug_info;
#else
    instance_info.pNext = nullptr;
#endif
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
#ifdef _DEBUG
    instance_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    instance_info.ppEnabledLayerNames = validation_layers;
#else
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = nullptr;
#endif
    instance_info.enabledExtensionCount = sizeof(enabled_extensions) / sizeof(*enabled_extensions);
    instance_info.ppEnabledExtensionNames = enabled_extensions;

    RB_VK(vkCreateInstance(&instance_info, nullptr, &_instance), "Cannot create Vulkan instance.");

    volkLoadInstance(_instance);
}

void graphics_vulkan::_choose_physical_device() {
    // Query physical device count. We should pick one.
    std::uint32_t physical_device_count{ 0 };
    RB_VK(vkEnumeratePhysicalDevices(_instance, &physical_device_count, nullptr),
        "Failed to query the number of physical devices.");

    // No supported physical devices?
    RB_ASSERT(physical_device_count > 0, "Couldn't detect any physical device with Vulkan support");

    // Enumerate through physical devices to pick one.
    auto physical_devices = std::make_unique<VkPhysicalDevice[]>(physical_device_count);
    RB_VK(vkEnumeratePhysicalDevices(_instance, &physical_device_count, physical_devices.get()),
        "Failed to enumarate physical devices");

    // TODO: We should find best device.
    _physical_device = physical_devices[0];

    vkGetPhysicalDeviceProperties(_physical_device, &_physical_device_properties);

    const auto counts = _physical_device_properties.limits.framebufferColorSampleCounts &
        _physical_device_properties.limits.framebufferDepthSampleCounts;

    //if (counts & VK_SAMPLE_COUNT_64_BIT) { _supported_samples = VK_SAMPLE_COUNT_64_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_32_BIT) { _supported_samples = VK_SAMPLE_COUNT_32_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_16_BIT) { _supported_samples = VK_SAMPLE_COUNT_16_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_8_BIT) { _supported_samples = VK_SAMPLE_COUNT_8_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_4_BIT) { _supported_samples = VK_SAMPLE_COUNT_4_BIT; }
    //else if (counts & VK_SAMPLE_COUNT_2_BIT) { _supported_samples = VK_SAMPLE_COUNT_2_BIT; }
}

void graphics_vulkan::_create_surface() {
#if RB_WINDOWS
    // Fill Win32 surface create informations.
    VkWin32SurfaceCreateInfoKHR surface_info;
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.pNext = nullptr;
    surface_info.flags = 0;
    surface_info.hinstance = GetModuleHandle(nullptr);
    surface_info.hwnd = window::native_handle();

    // Create new Vulkan surface.
    RB_VK(vkCreateWin32SurfaceKHR(_instance, &surface_info, nullptr, &_surface), "Failed to create Vulkan surface.");
#else
    RB_ASSERT(false, "Graphics for that platform is not implemented");
#endif
}

void graphics_vulkan::_create_device() {
    std::uint32_t queue_family_count{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, nullptr);

    auto queue_families = std::make_unique<VkQueueFamilyProperties[]>(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &queue_family_count, queue_families.get());

    for (std::uint32_t index{ 0 }; index < queue_family_count; ++index) {
        if (queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _graphics_family = index;
        }

        VkBool32 present_support{ VK_FALSE };
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, index, _surface, &present_support);

        if (present_support == VK_TRUE) {
            _present_family = index;
        }

        if (_graphics_family < UINT32_MAX && _present_family < UINT32_MAX) {
            // TODO: Should break?
            // break;
        }
    }

    // Fill queue priorities array.
    float queue_prorities[] = { 1.0f };

    // Fill graphics device queue create informations.
    VkDeviceQueueCreateInfo device_graphics_queue_info;
    device_graphics_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_graphics_queue_info.pNext = nullptr;
    device_graphics_queue_info.flags = 0;
    device_graphics_queue_info.queueFamilyIndex = _graphics_family;
    device_graphics_queue_info.queueCount = 1;
    device_graphics_queue_info.pQueuePriorities = queue_prorities;

    // Fill present queue create informations.
    VkDeviceQueueCreateInfo device_present_queue_info;
    device_present_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_present_queue_info.pNext = nullptr;
    device_present_queue_info.flags = 0;
    device_present_queue_info.queueFamilyIndex = _present_family;
    device_present_queue_info.queueCount = 1;
    device_present_queue_info.pQueuePriorities = queue_prorities;

    VkDeviceQueueCreateInfo device_queue_infos[] = {
        device_graphics_queue_info,
        device_present_queue_info
    };

    // Fill logical device extensions array.
    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME // Need swapchain to present render result onto a screen.
    };

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(_physical_device, &supported_features);

    // Fill device create informations.
    VkDeviceCreateInfo device_info;
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = nullptr;
    device_info.flags = 0;
#ifdef _DEBUG
    device_info.enabledLayerCount = sizeof(validation_layers) / sizeof(*validation_layers);
    device_info.ppEnabledLayerNames = validation_layers;
#else
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = nullptr;
#endif
    device_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(*device_extensions);
    device_info.ppEnabledExtensionNames = device_extensions;
    device_info.pEnabledFeatures = &supported_features;
    device_info.queueCreateInfoCount = sizeof(device_queue_infos) / sizeof(*device_queue_infos);
    device_info.pQueueCreateInfos = device_queue_infos;

    // Create new Vulkan logical device using physical one.
    RB_VK(vkCreateDevice(_physical_device, &device_info, nullptr, &_device), "Failed to create Vulkan logical device.");

    // Gets logical device queues.
    vkGetDeviceQueue(_device, _graphics_family, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, _present_family, 0, &_present_queue);
}

void graphics_vulkan::_create_allocator() {
    // Create Vulkan memory allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.instance = _instance;
    allocator_info.physicalDevice = _physical_device;
    allocator_info.device = _device;

    RB_VK(vmaCreateAllocator(&allocator_info, &_allocator), "Failed to create Vulkan memory allocator");
}

void graphics_vulkan::_query_surface() {
    // Query surface format count of picked physical device.
    std::uint32_t surface_format_count{ 0 };
    RB_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, nullptr),
        "Failed to query surface format count");

    // Enumarate all surface formats.
    auto surface_formats = std::make_unique<VkSurfaceFormatKHR[]>(surface_format_count);
    RB_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &surface_format_count, surface_formats.get()), 
        "Failed to enumerate surface formats");

    // Choose surface color format.
    if (surface_format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        _surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        _surface_format.format = surface_formats[0].format;
    }

    _surface_format.colorSpace = surface_formats[0].colorSpace;

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    RB_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities),
        "Failed to retrieve physical device surface capabilities");

    // Store swapchain extent
    _swapchain_extent = surface_capabilities.currentExtent;
}

void graphics_vulkan::_create_swapchain() {
    // Get window size.
    const auto window_size = window::size();

    // Query surface capabilities.
    VkSurfaceCapabilitiesKHR surface_capabilities;
    RB_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surface_capabilities),
        "Failed to retrieve physical device surface capabilities");

    std::uint32_t present_mode_count{ 0 };
    RB_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, nullptr),
        "Failed to query present mode count");

    auto present_modes = std::make_unique<VkPresentModeKHR[]>(present_mode_count);
    RB_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &present_mode_count, present_modes.get()),
        "Failed to enumerate present mode count");

    _present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    if (!settings::vsync) {
        _present_mode = VK_PRESENT_MODE_FIFO_KHR;
        _present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

        // Search for better solution.
        for (std::uint32_t index{ 0 }; index < present_mode_count; ++index) {
            if (present_modes[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
                _present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    auto image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }

    std::uint32_t queue_indices[] = { _graphics_family, _present_family };

    VkSwapchainCreateInfoKHR swapchain_info;
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.pNext = nullptr;
    swapchain_info.flags = 0;
    swapchain_info.surface = _surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = _surface_format.format;
    swapchain_info.imageColorSpace = _surface_format.colorSpace;
    swapchain_info.imageExtent = { window_size.x, window_size.y };
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (_graphics_family != _present_family) {
        swapchain_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(*queue_indices);
        swapchain_info.pQueueFamilyIndices = queue_indices;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    } else {
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapchain_info.presentMode = _present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.preTransform = surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.oldSwapchain = nullptr;

    // Create Vulkan swapchain.
    RB_VK(vkCreateSwapchainKHR(_device, &swapchain_info, nullptr, &_swapchain), "Failed to create swapchain");

    // Query swapchain image count.
    RB_VK(vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr), "Failed to query swapchain image count");

    // Get swapchain images list.
    _images.resize(image_count);
    RB_VK(vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _images.data()), "Failed to enumerate swapchain images");

    _image_views.resize(_images.size());
    for (std::size_t index{ 0 }; index < _images.size(); ++index) {
        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = _images[index];
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = _surface_format.format;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;
        RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_image_views[index]), "Failed to create image view");
    }

    const VkFormat depth_formats[] = {
        VK_FORMAT_D16_UNORM_S8_UINT
    };

    VkImageCreateInfo depth_image_info{};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.extent.width = window_size.x;
    depth_image_info.extent.height = window_size.y;
    depth_image_info.extent.depth = 1;
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.format = VK_FORMAT_D16_UNORM_S8_UINT; // VK_FORMAT_D24_UNORM_S8_UINT;
    depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo depth_allocation_info = {};
    depth_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &depth_image_info, &depth_allocation_info, &_depth_image, &_depth_image_allocation, nullptr),
        "Failed to create Vulkan depth image");

    VkImageViewCreateInfo depth_image_view_info{};
    depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_info.image = _depth_image;
    depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_info.format = VK_FORMAT_D16_UNORM_S8_UINT;
    depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_info.subresourceRange.levelCount = 1;
    depth_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &depth_image_view_info, nullptr, &_depth_image_view),
        "Failed to create Vulkan depth image view");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = _surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = VK_FORMAT_D16_UNORM_S8_UINT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference resolve_reference {};
    resolve_reference.attachment = 1;
    resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_reference;
    subpass.pResolveAttachments = nullptr;

    VkSubpassDependency subpass_dependencies[2]{};
    subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[0].dstSubpass = 0;
    subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[1].srcSubpass = 0;
    subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[0].dstSubpass = 0;
    subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; // Both stages might have access the depth-buffer, so need both in src/dstStageMask;;
    subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpass_dependencies[1].srcSubpass = 0;
    subpass_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpass_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = sizeof(attachments) / sizeof(*attachments);
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = subpass_dependencies;

    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass),
        "Failed to create render pass.");

    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_second_render_pass),
        "Failed to create render pass.");

    _framebuffers.resize(_image_views.size());
    for (std::size_t index{ 0 }; index < _images.size(); ++index) {
        VkImageView framebuffer_attachments[] = { _image_views[index], _depth_image_view };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = _render_pass;
        framebuffer_info.attachmentCount = sizeof(framebuffer_attachments) / sizeof(*framebuffer_attachments);
        framebuffer_info.pAttachments = framebuffer_attachments;
        framebuffer_info.width = window_size.x;
        framebuffer_info.height = window_size.y;
        framebuffer_info.layers = 1;

        RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_framebuffers[index]), 
            "Failed to create framebuffer.");
    }
}

void graphics_vulkan::_create_gbuffer() {
    VkFormat formats[] = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM
    };

    for (auto i = 0u; i < 3; ++i) {
        VkImageCreateInfo image_info;
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.pNext = nullptr;
        image_info.flags = 0;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = formats[i];
        image_info.extent = { _swapchain_extent.width, _swapchain_extent.height, 1 };
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = 0;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocation_info{};
        allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_gbuffer[i], &_gbuffer_allocations[i], nullptr),
            "Failed to create Vulkan image");

        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = _gbuffer[i];
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = formats[i];
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;
        RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_gbuffer_views[i]), "Failed to create image view");
    }

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // TODO: Do mapping.
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_gbuffer_sampler), "Failed to create Vulkan sampler");

    VkAttachmentDescription color_attachments[3];
    for (auto i = 0u; i < 3; ++i) {
        color_attachments[i].flags = 0;
        color_attachments[i].format = formats[i];
        color_attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkAttachmentReference color_attachment_references[3];
    for (auto i = 0u; i < 3; ++i) {
        color_attachment_references[i].attachment = i;
        color_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = VK_FORMAT_D16_UNORM_S8_UINT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 3;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc;
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = 3;
    subpass_desc.pColorAttachments = color_attachment_references;
    subpass_desc.pResolveAttachments = nullptr;
    subpass_desc.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkAttachmentDescription attachments[4]{
        color_attachments[0],
        color_attachments[1],
        color_attachments[2],
        depth_attachment
    };

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 4;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_gbuffer_render_pass),
        "Failed to create Vulkan render pass.");

    VkImageView image_views[4]{
        _gbuffer_views[0],
        _gbuffer_views[1],
        _gbuffer_views[2],
        _depth_image_view
    };

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = _gbuffer_render_pass;
    framebuffer_info.attachmentCount = 4;
    framebuffer_info.pAttachments = image_views;
    framebuffer_info.width = _swapchain_extent.width;
    framebuffer_info.height = _swapchain_extent.height;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_gbuffer_framebuffer),
        "Failed to create Vulkan framebuffer");

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(local_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[2]{
        _forward_descriptor_set_layout[0], // camera
        _forward_descriptor_set_layout[1] // material
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_gbuffer_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::geometry_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::geometry_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_gbuffer_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::geometry_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::geometry_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &_gbuffer_shader_modules[1]),
        "Failed to create shader module");

    struct forward_vertex {
        vec3f position;
        vec2f texcoord;
        vec3f normal;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(forward_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[3]{
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(forward_vertex, position) },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(forward_vertex, texcoord) },
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(forward_vertex, normal) }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 3;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain_extent.width);
    viewport.height = static_cast<float>(_swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_infos[]{
        color_blend_attachment_state_info,
        color_blend_attachment_state_info,
        color_blend_attachment_state_info
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 3;
    color_blend_state_info.pAttachments = color_blend_attachment_state_infos;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _gbuffer_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = _gbuffer_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _gbuffer_pipeline_layout;
    pipeline_info.renderPass = _gbuffer_render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_gbuffer_pipeline),
        "Failed to create Vulkan graphics pipeline");

    VkDescriptorSetLayoutBinding gbuffer_bindings[4]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 4;
    descriptor_set_layout_info.pBindings = gbuffer_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_gbuffer_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorPoolSize pool_sizes[1]{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 1;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_gbuffer_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _gbuffer_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_gbuffer_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_gbuffer_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorImageInfo image_infos[4]{
        { _gbuffer_sampler, _gbuffer_views[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _gbuffer_sampler, _gbuffer_views[1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _gbuffer_sampler, _gbuffer_views[2], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _gbuffer_sampler, _depth_image_view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL }
    };

    VkWriteDescriptorSet write_infos[4]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _gbuffer_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _gbuffer_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[1], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _gbuffer_descriptor_set, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[2], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _gbuffer_descriptor_set, 3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[3], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 4, write_infos, 0, nullptr);
}

void graphics_vulkan::_create_ambient_pipeline() {
    VkDescriptorSetLayout layouts[3]{
        _forward_descriptor_set_layout[0],
        _gbuffer_descriptor_set_layout,
        _forward_descriptor_set_layout[2],
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 3;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_ambient_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_ambient_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::ambient_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::ambient_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &_ambient_shader_modules[1]),
        "Failed to create shader module");

    struct quad_vertex {
        vec2f position;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(quad_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[1]{
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(quad_vertex, position) }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain_extent.width);
    viewport.height = static_cast<float>(_swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_FALSE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;
    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _ambient_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = _ambient_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _ambient_pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_ambient_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_create_directional_light_pipeline() {
    VkDescriptorSetLayout layouts[3]{
        _forward_descriptor_set_layout[0],
        _gbuffer_descriptor_set_layout,
        _forward_descriptor_set_layout[2],
    };

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(directional_light_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 3;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_directional_light_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule directional_light_shader_modules[2];

    VkShaderModuleCreateInfo directional_light_vert_shader_module_info;
    directional_light_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    directional_light_vert_shader_module_info.pNext = nullptr;
    directional_light_vert_shader_module_info.flags = 0;
    directional_light_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    directional_light_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &directional_light_vert_shader_module_info, nullptr, &directional_light_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo directional_light_frag_shader_module_info;
    directional_light_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    directional_light_frag_shader_module_info.pNext = nullptr;
    directional_light_frag_shader_module_info.flags = 0;
    directional_light_frag_shader_module_info.codeSize = shaders_vulkan::directional_light_frag().size_bytes();
    directional_light_frag_shader_module_info.pCode = shaders_vulkan::directional_light_frag().data();
    RB_VK(vkCreateShaderModule(_device, &directional_light_frag_shader_module_info, nullptr, &directional_light_shader_modules[1]),
        "Failed to create shader module");

    struct quad_vertex {
        vec2f position;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(quad_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[1]{
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(quad_vertex, position) }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain_extent.width);
    viewport.height = static_cast<float>(_swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_FALSE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_TRUE;
    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = directional_light_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = directional_light_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _directional_light_pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_directional_light_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, directional_light_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, directional_light_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_command_pool() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = _graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    RB_VK(vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool), "Failed to create command pool.");
}

void graphics_vulkan::_create_synchronization_objects() {
    VkSemaphoreCreateInfo semaphore_info;
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;

    RB_VK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_render_semaphore), "Failed to create render semaphore");
    RB_VK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_present_semaphore), "Failed to create present semaphore");
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, nullptr, &_image_index),
        "Failed to reset acquire next swapchain image");
}

void graphics_vulkan::_create_quad() {
    const vec2f vertices[] = {
        { -1.0f, -1.0f },
        { 1.0f, -1.0f },
        { 1.0f, 1.0f },
        { -1.0f, 1.0f },
    };

    const std::uint16_t indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    VkBufferCreateInfo vertex_buffer_info;
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.pNext = nullptr;
    vertex_buffer_info.flags = 0;
    vertex_buffer_info.size = sizeof(vertices);
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertex_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vertex_buffer_info.queueFamilyIndexCount = 0;
    vertex_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo vertex_allocation_info{};
    vertex_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &vertex_buffer_info, &vertex_allocation_info, &_quad_vertex_buffer, &_quad_vertex_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _quad_vertex_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, vertices, sizeof(vertices));
    vmaUnmapMemory(_allocator, _quad_vertex_allocation);

    VkBufferCreateInfo index_buffer_info;
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.pNext = nullptr;
    index_buffer_info.flags = 0;
    index_buffer_info.size = sizeof(indices);
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    index_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    index_buffer_info.queueFamilyIndexCount = 0;
    index_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo index_allocation_info{};
    index_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &index_buffer_info, &index_allocation_info, &_quad_index_buffer, &_quad_index_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    RB_VK(vmaMapMemory(_allocator, _quad_index_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, indices, sizeof(indices));
    vmaUnmapMemory(_allocator, _quad_index_allocation);
}


void graphics_vulkan::_generate_brdf_image() {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8_UNORM;
    image_info.extent = { graphics_limits::brdf_map_size, graphics_limits::brdf_map_size, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_brdf_image, &_brdf_allocation, nullptr),
        "Failed to create Vulkan image");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _brdf_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R8G8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_brdf_image_view),
        "Failed to create Vulkan image view");

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 0.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_brdf_sampler), "Failed to create Vulkan sampler");

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 0;
    descriptor_set_layout_info.pBindings = nullptr;

    VkDescriptorSetLayout descriptor_set_layout;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    VkPipelineLayout pipeline_layout;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::quad_vert().data();

    VkShaderModule vertex_shader_module;
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &vertex_shader_module),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::brdf_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::brdf_frag().data();

    VkShaderModule fragment_shader_module;
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &fragment_shader_module),
        "Failed to create shader module");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8G8_UNORM;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc;
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_reference;
    subpass_desc.pResolveAttachments = nullptr;
    subpass_desc.pDepthStencilAttachment = nullptr;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    VkSubpassDependency subpass_dependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;

    VkRenderPass render_pass;
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &render_pass),
        "Failed to create Vulkan render pass.");

    struct brdf_vertex {
        vec2f position;
    };
    
    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(brdf_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute;
    vertex_attribute.binding = 0;
    vertex_attribute.location = 0;
    vertex_attribute.format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attribute.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = &vertex_attribute;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = graphics_limits::brdf_map_size;
    viewport.height = graphics_limits::brdf_map_size;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { graphics_limits::brdf_map_size, graphics_limits::brdf_map_size };

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_FALSE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = vertex_shader_module;
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = fragment_shader_module;
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;

    VkPipeline pipeline;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline),
        "Failed to create Vulkan graphics pipeline");

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &_brdf_image_view;
    framebuffer_info.width = graphics_limits::brdf_map_size;
    framebuffer_info.height = graphics_limits::brdf_map_size;
    framebuffer_info.layers = 1;

    VkFramebuffer framebuffer;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &framebuffer),
        "Failed to create Vulkan framebuffer");

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = _command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(_device, &allocate_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkClearValue clear_values[1]{
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { graphics_limits::brdf_map_size, graphics_limits::brdf_map_size };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffer, _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_graphics_queue);

    vkFreeCommandBuffers(_device, _command_pool, 1, &command_buffer);

    vkDestroyFramebuffer(_device, framebuffer, nullptr);
    vkDestroyPipeline(_device, pipeline, nullptr);
    vkDestroyRenderPass(_device, render_pass, nullptr);
    vkDestroyShaderModule(_device, vertex_shader_module, nullptr);
    vkDestroyShaderModule(_device, fragment_shader_module, nullptr);
    vkDestroyPipelineLayout(_device, pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, descriptor_set_layout, nullptr);
}

void graphics_vulkan::_create_camera_buffer() {
    VkBufferCreateInfo camera_buffer_info;
    camera_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    camera_buffer_info.pNext = nullptr;
    camera_buffer_info.flags = 0;
    camera_buffer_info.size = sizeof(camera_data);
    camera_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    camera_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    camera_buffer_info.queueFamilyIndexCount = 0;
    camera_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo camera_allocation_info{};
    camera_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &camera_buffer_info, &camera_allocation_info, &_camera_buffer, &_camera_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    VkBufferCreateInfo light_buffer_info;
    light_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    light_buffer_info.pNext = nullptr;
    light_buffer_info.flags = 0;
    light_buffer_info.size = sizeof(light_list_data);
    light_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    light_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    light_buffer_info.queueFamilyIndexCount = 0;
    light_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo light_allocation_info{};
    light_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &light_buffer_info, &light_allocation_info, &_light_buffer, &_light_allocation, nullptr),
        "Failed to create Vulkan buffer.");
}

void graphics_vulkan::_create_skybox() {
    const vec3f vertices[24] = {
        { -1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f },

        { -1.0f, 1.0f, -1.0f },
        { -1.0f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f },
        { -1.0f, -1.0f, -1.0f },

        { 1.0f, 1.0f, -1.0f },
        { -1.0f, 1.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f },
        { 1.0f, -1.0f, -1.0f },

        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, -1.0f },
        { 1.0f, -1.0f, -1.0f },
        { 1.0f, -1.0f, 1.0f },

        { -1.0f, 1.0f, -1.0f },
        { 1.0f, 1.0f, -1.0f },
        { 1.0f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f },
        { 1.0f, -1.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f },
    };

    const std::uint16_t indices[36] = {
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4,

        8, 9, 10,
        10, 11, 8,

        12, 13, 14,
        14, 15, 12,

        16, 17, 18,
        18, 19, 16,

        20, 21, 22,
        22, 23, 20
    };

    VkBufferCreateInfo vertex_buffer_info;
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.pNext = nullptr;
    vertex_buffer_info.flags = 0;
    vertex_buffer_info.size = sizeof(vertices);
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertex_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vertex_buffer_info.queueFamilyIndexCount = 0;
    vertex_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo vertex_allocation_info{};
    vertex_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &vertex_buffer_info, &vertex_allocation_info, &_skybox_vertex_buffer, &_skybox_vertex_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _skybox_vertex_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, vertices, sizeof(vertices));
    vmaUnmapMemory(_allocator, _skybox_vertex_allocation);

    VkBufferCreateInfo index_buffer_info;
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.pNext = nullptr;
    index_buffer_info.flags = 0;
    index_buffer_info.size = sizeof(indices);
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    index_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    index_buffer_info.queueFamilyIndexCount = 0;
    index_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo index_allocation_info{};
    index_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &index_buffer_info, &index_allocation_info, &_skybox_index_buffer, &_skybox_index_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    RB_VK(vmaMapMemory(_allocator, _skybox_index_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, indices, sizeof(indices));
    vmaUnmapMemory(_allocator, _skybox_index_allocation);
}

void graphics_vulkan::_create_irradiance_pipeline() {
    VkDescriptorSetLayoutBinding bindings[2];
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 2;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_irradiance_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_irradiance_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _irradiance_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1; 
    descriptor_set_allocate_info.pSetLayouts = &_irradiance_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_irradiance_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &_irradiance_descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_irradiance_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::irradiance_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::irradiance_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_irradiance_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::irradiance_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::irradiance_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &_irradiance_shader_modules[1]),
        "Failed to create shader module");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc;
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_reference;
    subpass_desc.pResolveAttachments = nullptr;
    subpass_desc.pDepthStencilAttachment = nullptr;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    VkSubpassDependency subpass_dependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_irradiance_render_pass),
        "Failed to create Vulkan render pass.");

    struct irradiance_vertex {
        vec2f position;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(irradiance_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute;
    vertex_attribute.binding = 0;
    vertex_attribute.location = 0;
    vertex_attribute.format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attribute.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = &vertex_attribute;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = graphics_limits::irradiance_map_size;
    viewport.height = graphics_limits::irradiance_map_size;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { graphics_limits::irradiance_map_size, graphics_limits::irradiance_map_size };

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_FALSE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _irradiance_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = _irradiance_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _irradiance_pipeline_layout;
    pipeline_info.renderPass = _irradiance_render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_irradiance_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_bake_irradiance(const std::shared_ptr<environment>& environment) {
    const auto native_environment = std::static_pointer_cast<environment_vulkan>(environment);

    VkImageView irradiance_framebuffer_image_views[6];
    VkFramebuffer irradiance_framebuffers[6];

    for (auto i = 0; i < 6; ++i) {
        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = native_environment->irradiance_image();
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = i;
        image_view_info.subresourceRange.layerCount = 1;
        RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &irradiance_framebuffer_image_views[i]),
            "Failed to create Vulkan image view");

        VkFramebufferCreateInfo framebuffer_info;
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.pNext = nullptr;
        framebuffer_info.flags = 0;
        framebuffer_info.renderPass = _irradiance_render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = &irradiance_framebuffer_image_views[i];
        framebuffer_info.width = graphics_limits::irradiance_map_size;
        framebuffer_info.height = graphics_limits::irradiance_map_size;
        framebuffer_info.layers = 1;
        RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &irradiance_framebuffers[i]),
            "Failed to create Vulkan framebuffer");
    }

    VkBufferCreateInfo irradiance_buffer_info;
    irradiance_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    irradiance_buffer_info.pNext = nullptr;
    irradiance_buffer_info.flags = 0;
    irradiance_buffer_info.size = sizeof(irradiance_data);
    irradiance_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    irradiance_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    irradiance_buffer_info.queueFamilyIndexCount = 0;
    irradiance_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer irradiance_buffer;
    VmaAllocation irradiance_buffer_allocation;
    RB_VK(vmaCreateBuffer(_allocator, &irradiance_buffer_info, &allocation_info, &irradiance_buffer, &irradiance_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    VkWriteDescriptorSet write_infos[2];

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = irradiance_buffer;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(irradiance_data);

    VkDescriptorImageInfo image_info;
    image_info.sampler = native_environment->sampler();
    image_info.imageView = native_environment->image_view();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    write_infos[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[0].pNext = nullptr;
    write_infos[0].dstBinding = 0;
    write_infos[0].dstArrayElement = 0;
    write_infos[0].descriptorCount = 1;
    write_infos[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_infos[0].pBufferInfo = &buffer_info;
    write_infos[0].pImageInfo = nullptr;
    write_infos[0].pTexelBufferView = nullptr;
    write_infos[0].dstSet = _irradiance_descriptor_set;

    write_infos[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[1].pNext = nullptr;
    write_infos[1].dstBinding = 1;
    write_infos[1].dstArrayElement = 0;
    write_infos[1].descriptorCount = 1;
    write_infos[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_infos[1].pBufferInfo = nullptr;
    write_infos[1].pImageInfo = &image_info;
    write_infos[1].pTexelBufferView = nullptr;
    write_infos[1].dstSet = _irradiance_descriptor_set;

    vkUpdateDescriptorSets(_device, 2, write_infos, 0, nullptr);

    irradiance_data data;
    for (auto i = 0; i < 6; ++i) {
        // Create temporary buffer
        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = _command_pool;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(_device, &allocate_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info;
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begin_info.pInheritanceInfo = nullptr;

        // Begin registering commands
        vkBeginCommandBuffer(command_buffer, &begin_info);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _irradiance_pipeline);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _irradiance_pipeline_layout,
            0, 1, &_irradiance_descriptor_set, 0, nullptr);

        data.cube_face = i;
        vkCmdUpdateBuffer(command_buffer, irradiance_buffer, 0, sizeof(irradiance_data), &data);

        VkClearValue clear_values[1]{
            { 0.0f, 0.0f, 0.0f, 1.0f }
        };

        VkRenderPassBeginInfo render_pass_begin_info;
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = _irradiance_render_pass;
        render_pass_begin_info.framebuffer = irradiance_framebuffers[i];
        render_pass_begin_info.renderArea.offset = { 0, 0 };
        render_pass_begin_info.renderArea.extent = { graphics_limits::irradiance_map_size, graphics_limits::irradiance_map_size };
        render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
        render_pass_begin_info.pClearValues = clear_values;
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        const VkDeviceSize offset{ 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &_quad_vertex_buffer, &offset);
        vkCmdBindIndexBuffer(command_buffer, _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(command_buffer);

        // End registering commands
        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(_graphics_queue);

        vkFreeCommandBuffers(_device, _command_pool, 1, &command_buffer);
    }

    vmaDestroyBuffer(_allocator, irradiance_buffer, irradiance_buffer_allocation);

    for (auto i = 0; i < 6; ++i) {
        vkDestroyFramebuffer(_device, irradiance_framebuffers[i], nullptr);
        vkDestroyImageView(_device, irradiance_framebuffer_image_views[i], nullptr);
    }
}

void graphics_vulkan::_create_prefilter_pipeline() {
    VkDescriptorSetLayoutBinding bindings[2];
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 2;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_prefilter_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_prefilter_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _prefilter_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_prefilter_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_prefilter_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &_prefilter_descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_prefilter_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::prefilter_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::prefilter_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_prefilter_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::prefilter_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::prefilter_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &_prefilter_shader_modules[1]),
        "Failed to create shader module");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc;
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_reference;
    subpass_desc.pResolveAttachments = nullptr;
    subpass_desc.pDepthStencilAttachment = nullptr;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    VkSubpassDependency subpass_dependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_prefilter_render_pass),
        "Failed to create Vulkan render pass.");

    struct prefilter_vertex {
        vec2f position;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(prefilter_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute;
    vertex_attribute.binding = 0;
    vertex_attribute.location = 0;
    vertex_attribute.format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attribute.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = &vertex_attribute;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = graphics_limits::prefilter_map_size;
    viewport.height = graphics_limits::prefilter_map_size;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { graphics_limits::prefilter_map_size, graphics_limits::prefilter_map_size };

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_FALSE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _prefilter_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = _prefilter_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_state = VK_DYNAMIC_STATE_VIEWPORT;

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.flags = 0;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.dynamicStateCount = 1;
    dynamic_state_info.pDynamicStates = &dynamic_state;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _prefilter_pipeline_layout;
    pipeline_info.renderPass = _prefilter_render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_prefilter_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_bake_prefilter(const std::shared_ptr<environment>& environment) {
    const auto native_environment = std::static_pointer_cast<environment_vulkan>(environment);

    VkImageView prefilter_framebuffer_image_views[6][6];
    VkFramebuffer prefilter_framebuffers[6][6];

    vec2u resolution{ graphics_limits::prefilter_map_size, graphics_limits::prefilter_map_size };
    for (auto i = 0; i < 6; ++i) {
        for (auto j = 0; j < 6; ++j) {
            VkImageViewCreateInfo image_view_info;
            image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_info.pNext = nullptr;
            image_view_info.flags = 0;
            image_view_info.image = native_environment->prefilter_image();
            image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_info.subresourceRange.baseMipLevel = i;
            image_view_info.subresourceRange.levelCount = 1;
            image_view_info.subresourceRange.baseArrayLayer = j;
            image_view_info.subresourceRange.layerCount = 1;
            RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &prefilter_framebuffer_image_views[i][j]),
                "Failed to create Vulkan image view");

            VkFramebufferCreateInfo framebuffer_info;
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.pNext = nullptr;
            framebuffer_info.flags = 0;
            framebuffer_info.renderPass = _prefilter_render_pass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = &prefilter_framebuffer_image_views[i][j];
            framebuffer_info.width = resolution.x;
            framebuffer_info.height = resolution.y;
            framebuffer_info.layers = 1;
            RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &prefilter_framebuffers[i][j]),
                "Failed to create Vulkan framebuffer");
        }

        resolution.x = resolution.x / 2;
        resolution.y = resolution.y / 2;
    }

    VkBufferCreateInfo prefilter_buffer_info;
    prefilter_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    prefilter_buffer_info.pNext = nullptr;
    prefilter_buffer_info.flags = 0;
    prefilter_buffer_info.size = sizeof(prefilter_data);
    prefilter_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    prefilter_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    prefilter_buffer_info.queueFamilyIndexCount = 0;
    prefilter_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo prefilter_allocation_info{};
    prefilter_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer prefilter_buffer;
    VmaAllocation prefilter_buffer_allocation;
    RB_VK(vmaCreateBuffer(_allocator, &prefilter_buffer_info, &prefilter_allocation_info, &prefilter_buffer, &prefilter_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    VkWriteDescriptorSet write_infos[2];

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = prefilter_buffer;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(prefilter_data);

    VkDescriptorImageInfo image_info;
    image_info.sampler = native_environment->sampler();
    image_info.imageView = native_environment->image_view();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    write_infos[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[0].pNext = nullptr;
    write_infos[0].dstBinding = 0;
    write_infos[0].dstArrayElement = 0;
    write_infos[0].descriptorCount = 1;
    write_infos[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_infos[0].pBufferInfo = &buffer_info;
    write_infos[0].pImageInfo = nullptr;
    write_infos[0].pTexelBufferView = nullptr;
    write_infos[0].dstSet = _prefilter_descriptor_set;

    write_infos[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[1].pNext = nullptr;
    write_infos[1].dstBinding = 1;
    write_infos[1].dstArrayElement = 0;
    write_infos[1].descriptorCount = 1;
    write_infos[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_infos[1].pBufferInfo = nullptr;
    write_infos[1].pImageInfo = &image_info;
    write_infos[1].pTexelBufferView = nullptr;
    write_infos[1].dstSet = _prefilter_descriptor_set;

    vkUpdateDescriptorSets(_device, 2, write_infos, 0, nullptr);

    prefilter_data data;

    resolution = { graphics_limits::prefilter_map_size, graphics_limits::prefilter_map_size };
    for (auto i = 0; i < 6; ++i) {
        for (auto j = 0; j < 6; ++j) {
            // Create temporary buffer
            VkCommandBufferAllocateInfo allocate_info{};
            allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocate_info.commandPool = _command_pool;
            allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(_device, &allocate_info, &command_buffer);

            VkCommandBufferBeginInfo begin_info;
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.pNext = nullptr;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            begin_info.pInheritanceInfo = nullptr;

            // Begin registering commands
            vkBeginCommandBuffer(command_buffer, &begin_info);

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _prefilter_pipeline);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _prefilter_pipeline_layout,
                0, 1, &_prefilter_descriptor_set, 0, nullptr);

            data.cube_face = j;
            data.roughness = i / 6.0f;
            vkCmdUpdateBuffer(command_buffer, prefilter_buffer, 0, sizeof(prefilter_data), &data);

            VkClearValue clear_values[1]{
                { 0.0f, 0.0f, 0.0f, 1.0f }
            };

            VkViewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(resolution.x);
            viewport.height = static_cast<float>(resolution.y);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

            VkRenderPassBeginInfo render_pass_begin_info;
            render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_begin_info.pNext = nullptr;
            render_pass_begin_info.renderPass = _prefilter_render_pass;
            render_pass_begin_info.framebuffer = prefilter_framebuffers[i][j];
            render_pass_begin_info.renderArea.offset = { 0, 0 };
            render_pass_begin_info.renderArea.extent = { resolution.x, resolution.y };
            render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
            render_pass_begin_info.pClearValues = clear_values;
            vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            const VkDeviceSize offset{ 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, &_quad_vertex_buffer, &offset);
            vkCmdBindIndexBuffer(command_buffer, _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

            vkCmdEndRenderPass(command_buffer);

            // End registering commands
            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;

            vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(_graphics_queue);

            vkFreeCommandBuffers(_device, _command_pool, 1, &command_buffer);
        }

        resolution.x = resolution.x / 2;
        resolution.y = resolution.y / 2;
    }

    vmaDestroyBuffer(_allocator, prefilter_buffer, prefilter_buffer_allocation);
    for (auto i = 0; i < 6; ++i) {
        for (auto j = 0; j < 6; ++j) {
            vkDestroyFramebuffer(_device, prefilter_framebuffers[i][j], nullptr);
            vkDestroyImageView(_device, prefilter_framebuffer_image_views[i][j], nullptr);
        }
    }
}

void graphics_vulkan::_create_shadow_map() {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_D16_UNORM; 
    image_info.extent = { graphics_limits::shadow_map_size, graphics_limits::shadow_map_size, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_shadow_image, &_shadow_allocation, nullptr),
        "Failed to create Vulkan image.");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _shadow_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_D16_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_shadow_image_view),
        "Failed to create Vulkan image view");

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // TODO: Do mapping.
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_shadow_sampler), "Failed to create Vulkan sampler");

    VkAttachmentDescription attachment_description{};
    attachment_description.format = VK_FORMAT_D16_UNORM;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 0;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;													// No color attachments
    subpass.pDepthStencilAttachment = &depth_reference;									// Reference to our depth attachment

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &attachment_description;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_shadow_render_pass),
        "Failed to create render pass.");

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = _shadow_render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &_shadow_image_view;
    framebuffer_info.width = graphics_limits::shadow_map_size;
    framebuffer_info.height = graphics_limits::shadow_map_size;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_shadow_framebuffer),
        "Failed to create Vulkan framebuffer");

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(shadow_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_shadow_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::shadowmap_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::shadowmap_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_shadow_shader_module),
        "Failed to create shader module");

    struct forward_vertex {
        vec3f position;
        vec2f texcoord;
        vec3f normal;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(forward_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[3]{
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(forward_vertex, position) },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(forward_vertex, texcoord) },
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(forward_vertex, normal) }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 3;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = graphics_limits::shadow_map_size;
    viewport.height = graphics_limits::shadow_map_size;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { graphics_limits::shadow_map_size, graphics_limits::shadow_map_size };

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_TRUE;
    rasterizer_state_info.depthBiasConstantFactor = 1.25f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 1.75f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_info.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_CLEAR;
    color_blend_state_info.attachmentCount = 0;
    color_blend_state_info.pAttachments = nullptr;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _shadow_shader_module;
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 1;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _shadow_pipeline_layout;
    pipeline_info.renderPass = _shadow_render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_shadow_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_create_forward_pipeline() {
    VkDescriptorSetLayoutBinding bindings[4]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 4;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_forward_descriptor_set_layout[0]),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorSetLayoutBinding material_bindings[6]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo material_descriptor_set_layout_info;
    material_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    material_descriptor_set_layout_info.pNext = nullptr;
    material_descriptor_set_layout_info.flags = 0;
    material_descriptor_set_layout_info.bindingCount = 6;
    material_descriptor_set_layout_info.pBindings = material_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &material_descriptor_set_layout_info, nullptr, &_forward_descriptor_set_layout[1]),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorSetLayoutBinding environment_bindings[3]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 3;
    descriptor_set_layout_info.pBindings = environment_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_forward_descriptor_set_layout[2]),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * 2 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8*2 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 3;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_forward_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _forward_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_forward_descriptor_set_layout[0];
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_forward_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorBufferInfo buffer_infos[2]{
        { _camera_buffer, 0, sizeof(camera_data) },
        { _light_buffer, 0, sizeof(light_list_data) }
    };

    VkDescriptorImageInfo image_infos[2]{
        { _brdf_sampler, _brdf_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _shadow_sampler, _shadow_image_view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[4]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[0], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 2, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[1], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[1], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 4, write_infos, 0, nullptr);

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(local_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 3;
    pipeline_layout_info.pSetLayouts = _forward_descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_forward_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::forward_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::forward_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_forward_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::forward_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::forward_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &_forward_shader_modules[1]),
        "Failed to create shader module");

    struct forward_vertex {
        vec3f position;
        vec2f texcoord;
        vec3f normal;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(forward_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[3]{
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(forward_vertex, position) },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(forward_vertex, texcoord) },
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(forward_vertex, normal) }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 3;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain_extent.width);
    viewport.height = static_cast<float>(_swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _forward_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = _forward_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _forward_pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_forward_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_create_skybox_pipeline() {
    VkDescriptorSetLayout layouts[]{
        _forward_descriptor_set_layout[0],
        _forward_descriptor_set_layout[2],
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_skybox_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::skybox_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::skybox_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &_skybox_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::skybox_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::skybox_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &_skybox_shader_modules[1]),
        "Failed to create shader module");

    struct skybox_vertex {
        vec3f position;
    };

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(skybox_vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[1]{
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skybox_vertex, position) }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain_extent.width);
    viewport.height = static_cast<float>(_swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_info;
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.pNext = nullptr;
    viewport_state_info.flags = 0;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_info{};
    rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_info.pNext = nullptr;
    rasterizer_state_info.flags = 0;
    rasterizer_state_info.depthClampEnable = VK_FALSE;
    rasterizer_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = 0.0f;
    rasterizer_state_info.depthBiasClamp = 0.0f;
    rasterizer_state_info.depthBiasSlopeFactor = 0.0f;
    rasterizer_state_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_info;
    multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_info.pNext = nullptr;
    multisampling_state_info.flags = 0;
    multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_info.sampleShadingEnable = VK_FALSE;
    multisampling_state_info.minSampleShading = 0.0f;
    multisampling_state_info.pSampleMask = nullptr;
    multisampling_state_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.pNext = nullptr;
    depth_stencil_state_info.flags = 0;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment_state_info;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = _skybox_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = _skybox_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterizer_state_info;
    pipeline_info.pMultisampleState = &multisampling_state_info;
    pipeline_info.pColorBlendState = &color_blend_state_info;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;
    pipeline_info.layout = _skybox_pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_skybox_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_create_command_buffers() {
    VkCommandBufferAllocateInfo command_buffer_alloc_info;
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = _command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 3;

    RB_VK(vkAllocateCommandBuffers(_device, &command_buffer_alloc_info, _command_buffers),
        "Failed to allocate command buffer");

    VkFenceCreateInfo fence_info;
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& fence : _fences) {
        RB_VK(vkCreateFence(_device, &fence_info, nullptr, &fence), "Failed to create fence");
    }
}

VkCommandBuffer graphics_vulkan::_command_begin() {
    _command_index = (_command_index + 1) % 3;

    RB_VK(vkWaitForFences(_device, 1, &_fences[_command_index], VK_TRUE, 1000000000),
        "Failed to wait for render fence");

    RB_VK(vkResetFences(_device, 1, &_fences[_command_index]), "Failed to reset render fence");

    // Now that we are sure that the commands finished executing,
    // we can safely reset the command buffer to begin recording again.
    RB_VK(vkResetCommandBuffer(_command_buffers[_command_index], 0), "Failed to reset command buffer");

    // Begin the command buffer recording.
    // We will use this command buffer exactly once, so we want to let Vulkan know that.
    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    RB_VK(vkBeginCommandBuffer(_command_buffers[_command_index], &begin_info), "Failed to begin command buffer");
    return _command_buffers[_command_index];
}

void graphics_vulkan::_command_end() {
    // Finalize the command buffer (we can no longer add commands, but it can now be executed)
    RB_VK(vkEndCommandBuffer(_command_buffers[_command_index]), "Failed to end command buffer");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_command_buffers[_command_index];
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    RB_VK(vkQueueSubmit(_graphics_queue, 1, &submit_info, _fences[_command_index]), "Failed to queue submit");
}
