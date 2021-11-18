#include "graphics_vulkan.hpp"
#include "viewport_vulkan.hpp"
#include "texture_vulkan.hpp"
#include "environment_vulkan.hpp"
#include "material_vulkan.hpp"
#include "mesh_vulkan.hpp"
#include "shaders_vulkan.hpp"
#include "utils_vulkan.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <random>

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
    _create_skybox();
    _create_irradiance_pipeline();
    _create_prefilter_pipeline();
    _create_shadow_map();
    _create_camera();
    _create_main();
    _create_material();
    _create_environment();
    _create_depth();
    _create_light();
    _create_forward();
    _create_postprocess();
    //_create_ssao_pipeline();
    _create_fxaa_pipeline();
    _create_blur_pipeline();
    _create_sharpen_pipeline();
    _create_motion_blur_pipeline();
    _create_fill_pipeline();
    _create_outline_pipeline();
    _create_skybox_pipeline();
    _create_present_pipeline();
    _create_command_buffers();
}

graphics_vulkan::~graphics_vulkan() {
    vkQueueWaitIdle(_graphics_queue);
    vkQueueWaitIdle(_present_queue);
    vkDeviceWaitIdle(_device);

    vkDestroyPipeline(_device, _forward_copy_pipeline, nullptr);
    vkDestroyPipeline(_device, _light_copy_pipeline, nullptr);
    vkDestroyPipeline(_device, _present_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _present_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _outline_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _outline_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _fill_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _fill_pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _fill_descriptor_set_layout, nullptr);
    vkDestroyRenderPass(_device, _fill_render_pass, nullptr);

    vkDestroyPipeline(_device, _motion_blur_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _motion_blur_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _sharpen_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _sharpen_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _blur_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _blur_pipeline_layout, nullptr);

    vkDestroyPipeline(_device, _fxaa_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _fxaa_pipeline_layout, nullptr);

    //vkDestroyPipeline(_device, _ssao_blur_pipeline, nullptr);
    //vkDestroyPipelineLayout(_device, _ssao_blur_pipeline_layout, nullptr);
    //vkDestroyPipeline(_device, _ssao_pipeline, nullptr);
    //vkDestroyPipelineLayout(_device, _ssao_pipeline_layout, nullptr);
    //vkDestroyDescriptorPool(_device, _ssao_descriptor_pool, nullptr);
    //vkDestroyDescriptorSetLayout(_device, _ssao_descriptor_set_layout, nullptr);
    //vkDestroyDescriptorSetLayout(_device, _ssao_blur_descriptor_set_layout, nullptr);
    //vkDestroyFramebuffer(_device, _ssao_framebuffer, nullptr);
    //vkDestroyRenderPass(_device, _ssao_render_pass, nullptr);
    //vkDestroyImageView(_device, _ssao_image_view, nullptr);
    //vmaDestroyImage(_allocator, _ssao_image, _ssao_image_allocation);
    //vkDestroySampler(_device, _ssao_sampler, nullptr);
    //vkDestroyImageView(_device, _ssao_noise_map_view, nullptr);
    //vmaDestroyImage(_allocator, _ssao_noise_map, _ssao_noise_map_allocation);
    //vmaDestroyBuffer(_allocator, _ssao_buffer, _ssao_allocation);

    vkDestroyPipeline(_device, _skybox_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _skybox_pipeline_layout, nullptr);

    vkDestroyDescriptorSetLayout(_device, _postprocess_descriptor_set_layout, nullptr);
    vkDestroyRenderPass(_device, _postprocess_render_pass, nullptr);

    for (const auto& [flags, pipeline] : _forward_pipelines) {
        vkDestroyPipeline(_device, pipeline, nullptr);
    }
    for (const auto& [flags, pipeline_layout] : _forward_pipeline_layouts) {
        vkDestroyPipelineLayout(_device, pipeline_layout, nullptr);
    }
    vkDestroyDescriptorSetLayout(_device, _forward_descriptor_set_layout, nullptr);
    vkDestroyRenderPass(_device, _forward_render_pass, nullptr);

    vkDestroyDescriptorSetLayout(_device, _light_descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(_device, _light_pipeline_layout, nullptr);
    vkDestroyPipeline(_device, _light_pipeline, nullptr);

    vkDestroyDescriptorSetLayout(_device, _depth_descriptor_set_layout, nullptr);
    vkDestroyRenderPass(_device, _depth_render_pass, nullptr);
    vkDestroyPipelineLayout(_device, _depth_pipeline_layout, nullptr);
    vkDestroyPipeline(_device, _depth_pipeline, nullptr);

    vkDestroyDescriptorSetLayout(_device, _environment_descriptor_set_layout, nullptr);

    vkDestroyDescriptorSetLayout(_device, _material_descriptor_set_layout, nullptr);

    vkDestroyDescriptorSetLayout(_device, _main_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(_device, _main_descriptor_pool, nullptr);

    vmaDestroyBuffer(_allocator, _camera_buffer, _camera_allocation);

    vkDestroyPipeline(_device, _shadow_pipeline, nullptr);
    vkDestroyShaderModule(_device, _shadow_shader_module, nullptr);
    vkDestroyPipelineLayout(_device, _shadow_pipeline_layout, nullptr);
    for (auto i = 0u; i < graphics_limits::max_shadow_cascades; ++i) {
        vkDestroyFramebuffer(_device, _shadow_framebuffers[i], nullptr);
    }
    vkDestroyRenderPass(_device, _shadow_render_pass, nullptr);
    vkDestroySampler(_device, _shadow_sampler, nullptr);
    vkDestroyImageView(_device, _shadow_image_view, nullptr);
    for (auto i = 0u; i < graphics_limits::max_shadow_cascades; ++i) {
        vkDestroyImageView(_device, _shadow_image_views[i], nullptr);
    }
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

    vkDestroySampler(_device, _brdf_sampler, nullptr);
    vkDestroyImageView(_device, _brdf_image_view, nullptr);
    vmaDestroyImage(_allocator, _brdf_image, _brdf_allocation);

    vmaDestroyBuffer(_allocator, _quad_vertex_buffer, _quad_vertex_allocation);
    vmaDestroyBuffer(_allocator, _quad_index_buffer, _quad_index_allocation);

    vkDestroySemaphore(_device, _present_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_semaphore, nullptr);
    vkDestroyCommandPool(_device, _command_pool, nullptr);
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

std::shared_ptr<rb::viewport> graphics_vulkan::make_viewport(const viewport_desc& desc) {
    return std::make_shared<viewport_vulkan>(_device,
        _allocator,
        _get_supported_depth_format(),
        _depth_render_pass,
        _depth_descriptor_set_layout,
        _light_descriptor_set_layout,
        _forward_render_pass,
        _forward_descriptor_set_layout,
        _postprocess_render_pass,
        _postprocess_descriptor_set_layout,
        _fill_render_pass,
        _fill_descriptor_set_layout,
        desc);
}

std::shared_ptr<texture> graphics_vulkan::make_texture(const texture_desc& desc) {
    return std::make_shared<texture_vulkan>(_device, _physical_device_properties, _graphics_queue, _command_pool, _allocator, desc);
}

std::shared_ptr<environment> graphics_vulkan::make_environment(const environment_desc& desc) {
    const auto environment = std::make_shared<environment_vulkan>(_device, _graphics_queue, _command_pool, _allocator, _environment_descriptor_set_layout, desc);
    _bake_irradiance(environment);
    _bake_prefilter(environment);
    return environment;
}

std::shared_ptr<material> graphics_vulkan::make_material(const material_desc& desc) {
	return std::make_shared<material_vulkan>(_device, _allocator, desc);
}

std::shared_ptr<mesh> graphics_vulkan::make_mesh(const mesh_desc& desc) {
    return std::make_shared<mesh_vulkan>(_device, _allocator, desc);
}

void graphics_vulkan::begin() {
    _command_begin();
}

void graphics_vulkan::set_camera(const mat4f& projection, const mat4f& view, const mat4f& world, const std::shared_ptr<environment>& environment) {
    _environment = std::static_pointer_cast<environment_vulkan>(environment);

    const auto aspect = static_cast<float>(_swapchain_extent.width) / _swapchain_extent.height;
    if (_camera_data.last_proj_view != mat4f::identity()) {
        _camera_data.last_proj_view = _camera_data.projection * _camera_data.view;
    }
    _camera_data.projection = projection;
    _camera_data.view = view;
    if (_camera_data.last_proj_view == mat4f::identity()) {
        _camera_data.last_proj_view = _camera_data.projection * _camera_data.view;
    }
    _camera_data.inv_proj_view = invert(_camera_data.projection * _camera_data.view);
    _camera_data.camera_position = { world[12], world[13], world[14] };

    vkCmdUpdateBuffer(_command_buffers[_command_index], _camera_buffer, 0, sizeof(camera_data), &_camera_data);
}

void graphics_vulkan::begin_depth_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    native_viewport->begin_depth_pass(_command_buffers[_command_index]);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _depth_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _depth_pipeline);
}

void graphics_vulkan::draw_depth(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, std::size_t mesh_lod_index) {
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(mesh);

    VkDeviceSize offset{ 0 };
    VkBuffer buffer{ native_mesh->vertex_buffer() };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], native_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(_command_buffers[_command_index], _depth_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4f), &world);
    
    const auto& lod = native_mesh->lods()[mesh_lod_index];
    vkCmdDrawIndexed(_command_buffers[_command_index], lod.size, 1, lod.offset, 0, 0);
}

void graphics_vulkan::end_depth_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->end_depth_pass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, std::size_t cascade) {
    const auto factor = static_cast<float>(1 << cascade);

    const auto dir = normalize(transform_normal(mat4f::rotation(transform.rotation), vec3f::z_axis()));
    const auto depth_projection = mat4f::orthographic(-10.0f * factor, 10.0f * factor, -10.0f * factor, 10.0f * factor, -20.0f * factor, 20.0f * factor);
    const auto depth_view = mat4f::look_at(_camera_data.camera_position - dir * 10.0f * factor, _camera_data.camera_position, vec3f::up());
    _camera_data.light_proj_view[cascade] = depth_projection * depth_view;

    VkClearValue clear_values[1];
    clear_values[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _shadow_render_pass;
    render_pass_begin_info.framebuffer = _shadow_framebuffers[cascade];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { graphics_limits::shadow_map_size, graphics_limits::shadow_map_size };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _shadow_pipeline);
}

void graphics_vulkan::draw_shadow(const mat4f& world, const geometry& geometry, std::size_t cascade) {
    const auto native_material = std::static_pointer_cast<material_vulkan>(geometry.material);
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(geometry.mesh);

    VkDeviceSize offset{ 0 };
    VkBuffer buffer{ native_mesh->vertex_buffer() };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], native_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

    shadow_data shadow_data;
    shadow_data.proj_view_world = _camera_data.light_proj_view[cascade] * world;
    vkCmdPushConstants(_command_buffers[_command_index], _shadow_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shadow_data), &shadow_data);

    const auto& lod = native_mesh->lods().back();
    vkCmdDrawIndexed(_command_buffers[_command_index], lod.size, 1, lod.offset, 0, 0);
}

void graphics_vulkan::end_shadow_pass() {
    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_light_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->begin_light_pass(_command_buffers[_command_index]);
}

void graphics_vulkan::add_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->add_point_light(transform.position, point_light.radius, 
        vec3f{ light.color.r / 255.0f, light.color.g / 255.0f, light.color.b / 255.0f } * light.intensity);
}

void graphics_vulkan::add_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light, bool use_shadow) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    const auto dir = normalize(transform_normal(mat4f::rotation(transform.rotation), vec3f::z_axis()));
    native_viewport->add_directional_light(dir,
        vec3f{ light.color.r / 255.0f, light.color.g / 255.0f, light.color.b / 255.0f } * light.intensity,
        directional_light.shadow_enabled && use_shadow);
}

void graphics_vulkan::end_light_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->end_light_pass(_command_buffers[_command_index]);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set,
        native_viewport->depth_descriptor_set(),
        native_viewport->light_descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_COMPUTE, _light_pipeline_layout, 0, 3, descriptor_sets,
        0, nullptr);

    const auto work_groups_x = (viewport->size().x + (viewport->size().x % 16)) / 16;
    const auto work_groups_y = (viewport->size().y + (viewport->size().y % 16)) / 16;
    const auto number_of_tiles = work_groups_x * work_groups_y;

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_COMPUTE, _light_pipeline);

    VkBufferMemoryBarrier barrier;
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = _graphics_family;
    barrier.dstQueueFamilyIndex = _graphics_family;
    barrier.buffer = native_viewport->visible_light_indices_buffer();
    barrier.offset = 0;
    barrier.size = number_of_tiles * sizeof(int) * graphics_limits::max_lights;
    vkCmdPipelineBarrier(_command_buffers[_command_index],
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 1, &barrier, 0, nullptr);

    vkCmdDispatch(_command_buffers[_command_index], work_groups_x, work_groups_y, 1);

    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = _graphics_family;
    barrier.dstQueueFamilyIndex = _graphics_family;
    barrier.buffer = native_viewport->visible_light_indices_buffer();
    barrier.offset = 0;
    barrier.size = number_of_tiles * sizeof(int) * graphics_limits::max_lights;
    vkCmdPipelineBarrier(_command_buffers[_command_index],
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void graphics_vulkan::begin_forward_pass(const std::shared_ptr<viewport>& viewport) {
    vkCmdUpdateBuffer(_command_buffers[_command_index], _camera_buffer,
        offsetof(camera_data, light_proj_view),
        sizeof(_camera_data.light_proj_view),
        &_camera_data.light_proj_view);

    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->begin_forward_pass(_command_buffers[_command_index]);
}

void graphics_vulkan::draw_skybox(const std::shared_ptr<viewport>& viewport) {
    if (!_environment) {
        return;
    }

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _skybox_pipeline);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set,
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

void graphics_vulkan::draw_forward(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material, std::size_t mesh_lod_index) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    const auto native_material = std::static_pointer_cast<material_vulkan>(material);
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(mesh);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set,
        native_material->descriptor_set(),
        _environment->descriptor_set(),
        native_viewport->light_descriptor_set()
    };

    const auto pipeline_layout = _get_forward_pipeline_layout(native_material);
    const auto pipeline = _get_forward_pipeline(native_material);

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 4, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    VkBuffer buffer{ native_mesh->vertex_buffer() };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], native_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(_command_buffers[_command_index], pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4f), &world);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const auto& lod = native_mesh->lods()[mesh_lod_index];
    vkCmdDrawIndexed(_command_buffers[_command_index], lod.size, 1, lod.offset, 0, 0);
}

void graphics_vulkan::end_forward_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->end_forward_pass(_command_buffers[_command_index]);
}

void graphics_vulkan::pre_draw_ssao(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkClearValue clear_values[1];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _ssao_render_pass;
    render_pass_begin_info.framebuffer = _ssao_framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { _swapchain_extent.width / (unsigned int)graphics_limits::ssao_image_reduction, _swapchain_extent.height / (unsigned int)graphics_limits::ssao_image_reduction };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set,
        native_viewport->depth_descriptor_set(),
        _ssao_descriptor_set
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _ssao_pipeline_layout, 0, 3, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _ssao_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_fill_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkClearValue clear_values[1];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _fill_render_pass;
    render_pass_begin_info.framebuffer = native_viewport->fill_framebuffer();
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { viewport->size().x, viewport->size().y };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _fill_pipeline);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _fill_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);
}

void graphics_vulkan::draw_fill(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry) {
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(geometry.mesh);

    VkDeviceSize offset{ 0 };
    VkBuffer buffer{ native_mesh->vertex_buffer() };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], native_mesh->index_buffer(), 0, VK_INDEX_TYPE_UINT32);

    local_data local_data;
    local_data.world = mat4f::translation(transform.position) *
        mat4f::rotation(transform.rotation) *
        mat4f::scaling(transform.scaling);

    vkCmdPushConstants(_command_buffers[_command_index], _fill_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(local_data), &local_data);

    vkCmdDrawIndexed(_command_buffers[_command_index], static_cast<std::uint32_t>(native_mesh->indices().size()), 1, 0, 0, 0);
}

void graphics_vulkan::end_fill_pass(const std::shared_ptr<viewport>& viewport) {
    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_postprocess_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->begin_postprocess_pass(_command_buffers[_command_index]);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->forward_descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _present_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _forward_copy_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::next_postprocess_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->end_postprocess_pass(_command_buffers[_command_index]);
    native_viewport->begin_postprocess_pass(_command_buffers[_command_index]);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->postprocess_descriptor_set()
    };
}

void graphics_vulkan::draw_ssao(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->postprocess_descriptor_set(),
        _ssao_blur_descriptor_set
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _ssao_blur_pipeline_layout, 0, 2, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    blur_data blur_data;
    blur_data.strength = 2;
    vkCmdPushConstants(_command_buffers[_command_index], _ssao_blur_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(blur_data), &blur_data);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _ssao_blur_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_fxaa(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->postprocess_descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _fxaa_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _fxaa_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_blur(const std::shared_ptr<viewport>& viewport, int strength) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->postprocess_descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _blur_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    blur_data blur_data;
    blur_data.strength = strength;
    vkCmdPushConstants(_command_buffers[_command_index], _blur_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(blur_data), &blur_data);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _blur_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->postprocess_descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _sharpen_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    sharpen_data sharpen_data;
    sharpen_data.strength = strength;
    vkCmdPushConstants(_command_buffers[_command_index], _sharpen_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(sharpen_data), &sharpen_data);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _sharpen_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_motion_blur(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkDescriptorSet descriptor_sets[]{
        _main_descriptor_set,
        native_viewport->depth_descriptor_set(),
        native_viewport->postprocess_descriptor_set(),
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _motion_blur_pipeline_layout, 0, 3, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _motion_blur_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::draw_outline(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->fill_descriptor_set(),
        native_viewport->postprocess_descriptor_set(),
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _outline_pipeline_layout, 0, 2, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _outline_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);
}

void graphics_vulkan::end_postprocess_pass(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);
    native_viewport->end_postprocess_pass(_command_buffers[_command_index]);
}

void graphics_vulkan::begin_immediate_pass() {

}

void graphics_vulkan::draw_immediate_color(const span<const vertex>& vertices, const color& color) {

}

void graphics_vulkan::draw_immediate_textured(const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) {

}

void graphics_vulkan::end_immediate_pass() {

}

void graphics_vulkan::end() {
    _command_end();
}

void graphics_vulkan::present(const std::shared_ptr<viewport>& viewport) {
    const auto native_viewport = std::static_pointer_cast<viewport_vulkan>(viewport);

    // No need to clear depth buffer because we will resue it from gbuffer
    VkClearValue clear_values[2];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clear_values[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.framebuffer = _framebuffers[_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = _swapchain_extent;
    render_pass_begin_info.clearValueCount = 2; // sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(_command_buffers[_command_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewports[]{
        0.0f, 0.0f,
        static_cast<float>(_swapchain_extent.width), static_cast<float>(_swapchain_extent.height),
        0.0f, 1.0f
    };
    vkCmdSetViewport(_command_buffers[_command_index], 0, 1, viewports);

    VkRect2D scissor{ { 0, 0 }, _swapchain_extent };
    vkCmdSetScissor(_command_buffers[_command_index], 0, 1, &scissor);

    VkDescriptorSet descriptor_sets[]{
        native_viewport->postprocess_descriptor_set()
    };

    vkCmdBindDescriptorSets(_command_buffers[_command_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS, _present_pipeline_layout, 0, 1, descriptor_sets,
        0, nullptr);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(_command_buffers[_command_index], 0, 1, &_quad_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(_command_buffers[_command_index], _quad_index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindPipeline(_command_buffers[_command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, _present_pipeline);
    vkCmdDrawIndexed(_command_buffers[_command_index], 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(_command_buffers[_command_index]);
}

void graphics_vulkan::swap_buffers() {
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
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, VK_NULL_HANDLE, &_image_index), "Failed to reset acquire next swapchain image");
}

void graphics_vulkan::flush() {
    for (auto& fence : _fences) {
        vkWaitForFences(_device, 1, &fence, VK_TRUE, 1000000000);
        vkDestroyFence(_device, fence, nullptr);
    }
    vkFreeCommandBuffers(_device, _command_pool, max_command_buffers, _command_buffers);

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

    _present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (!settings::vsync) {
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
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

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

    const auto depth_format = _get_supported_depth_format();

    const VkFormat depth_formats[] = {
        depth_format
    };

    VkImageCreateInfo depth_image_info{};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.extent.width = window_size.x;
    depth_image_info.extent.height = window_size.y;
    depth_image_info.extent.depth = 1;
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.format = depth_format; // VK_FORMAT_D24_UNORM_S8_UINT;
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
    depth_image_view_info.format = depth_format;
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
    depth_attachment.format = depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

void graphics_vulkan::_create_camera() {
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
    camera_allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &camera_buffer_info, &camera_allocation_info, &_camera_buffer, &_camera_allocation, nullptr),
        "Failed to create Vulkan buffer.");
}

void graphics_vulkan::_create_main() {
    VkDescriptorSetLayoutBinding bindings[3]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 3;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_main_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_main_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _main_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_main_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_main_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorBufferInfo buffer_infos[1]{
        { _camera_buffer, 0, sizeof(camera_data) },
    };

    VkDescriptorImageInfo image_infos[2]{
        { _brdf_sampler, _brdf_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _shadow_sampler, _shadow_image_view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[3]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _main_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[0], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _main_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _main_descriptor_set, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[1], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 3, write_infos, 0, nullptr);
}

void graphics_vulkan::_create_material() {
    VkDescriptorSetLayoutBinding material_bindings[7]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo material_descriptor_set_layout_info;
    material_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    material_descriptor_set_layout_info.pNext = nullptr;
    material_descriptor_set_layout_info.flags = 0;
    material_descriptor_set_layout_info.bindingCount = 7;
    material_descriptor_set_layout_info.pBindings = material_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &material_descriptor_set_layout_info, nullptr, &_material_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");
}

void graphics_vulkan::_create_environment() {
    VkDescriptorSetLayoutBinding environment_bindings[3]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 3;
    descriptor_set_layout_info.pBindings = environment_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_environment_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");
}

void graphics_vulkan::_create_depth() {
    const auto depth_format = _get_supported_depth_format();

    VkAttachmentDescription attachment_description{};
    attachment_description.format = depth_format;
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
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
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
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_depth_render_pass),
        "Failed to create render pass.");

    VkDescriptorSetLayout layouts[]{
        _main_descriptor_set_layout
    };

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(local_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_depth_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule depth_shader_module;

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::depth_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::depth_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &depth_shader_module),
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
    scissor.extent = { _swapchain_extent.width, _swapchain_extent.height };

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
    rasterizer_state_info.depthBiasConstantFactor = 1.00f;
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
    vertex_shader_stage_info.module = depth_shader_module;
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _depth_pipeline_layout;
    pipeline_info.renderPass = _depth_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_depth_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, depth_shader_module, nullptr);

    VkDescriptorSetLayoutBinding depth_bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo depth_descriptor_set_layout_info;
    depth_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    depth_descriptor_set_layout_info.pNext = nullptr;
    depth_descriptor_set_layout_info.flags = 0;
    depth_descriptor_set_layout_info.bindingCount = 1;
    depth_descriptor_set_layout_info.pBindings = depth_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &depth_descriptor_set_layout_info, nullptr, &_depth_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");
}

void graphics_vulkan::_create_light() {
    VkDescriptorSetLayoutBinding light_bindings[3]{
        { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo light_descriptor_set_layout_info;
    light_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    light_descriptor_set_layout_info.pNext = nullptr;
    light_descriptor_set_layout_info.flags = 0;
    light_descriptor_set_layout_info.bindingCount = 3;
    light_descriptor_set_layout_info.pBindings = light_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &light_descriptor_set_layout_info, nullptr, &_light_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorSetLayout layouts[3]{
        _main_descriptor_set_layout,
        _depth_descriptor_set_layout,
        _light_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 3;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    VkPipelineLayout pipeline_layout;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_light_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModuleCreateInfo shader_module_create_info;
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.flags = 0;
    shader_module_create_info.pNext = nullptr;
    shader_module_create_info.pCode = shaders_vulkan::light_cull_comp().data();
    shader_module_create_info.codeSize = shaders_vulkan::light_cull_comp().size_bytes();

    VkShaderModule shader_module;
    RB_VK(vkCreateShaderModule(_device, &shader_module_create_info, nullptr, &shader_module),
        "Failed to create Vulkan shader module");

    VkComputePipelineCreateInfo compute_pipeline_create_info;
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = nullptr;
    compute_pipeline_create_info.flags = 0;
    compute_pipeline_create_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compute_pipeline_create_info.stage.pNext = nullptr;
    compute_pipeline_create_info.stage.flags = 0;
    compute_pipeline_create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compute_pipeline_create_info.stage.module = shader_module;
    compute_pipeline_create_info.stage.pName = "main";
    compute_pipeline_create_info.stage.pSpecializationInfo = nullptr;
    compute_pipeline_create_info.layout = _light_pipeline_layout;
    compute_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    compute_pipeline_create_info.basePipelineIndex = 0;
    RB_VK(vkCreateComputePipelines(_device, nullptr, 1, &compute_pipeline_create_info, nullptr, &_light_pipeline),
        "Failed to create Vulkan compute pipeline");

    vkDestroyShaderModule(_device, shader_module, nullptr);
}

VkPipelineLayout graphics_vulkan::_create_forward_pipeline_layout(const std::shared_ptr<material>& material) {
    const auto native_material = std::static_pointer_cast<material_vulkan>(material);

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(local_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[4]{
        _main_descriptor_set_layout, // main
        native_material->descriptor_set_layout(), // material
        _environment_descriptor_set_layout,
        _light_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 4;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    VkPipelineLayout pipeline_layout;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    return pipeline_layout;
}

VkPipelineLayout graphics_vulkan::_get_forward_pipeline_layout(const std::shared_ptr<material>& material) {
    const auto flags = material ? material->flags() : 0;
    auto& pipeline_layout = _forward_pipeline_layouts[flags];
    if (pipeline_layout) {
        return pipeline_layout;
    }
    return pipeline_layout = _create_forward_pipeline_layout(material);
}

VkPipeline graphics_vulkan::_create_forward_pipeline(const std::shared_ptr<material>& material) {
    const auto native_material = std::static_pointer_cast<material_vulkan>(material);

    const auto pipeline_layout = _get_forward_pipeline_layout(material);

    VkShaderModule forward_shader_modules[2];

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::forward_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::forward_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &forward_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    if (material->flags() & material_flags::all_maps_bits) {
        fragment_shader_module_info.codeSize = shaders_vulkan::forward_frag().size_bytes();
        fragment_shader_module_info.pCode = shaders_vulkan::forward_frag().data();
    } else {
        fragment_shader_module_info.codeSize = shaders_vulkan::forward_nomaps_frag().size_bytes();
        fragment_shader_module_info.pCode = shaders_vulkan::forward_nomaps_frag().data();
    }
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &forward_shader_modules[1]),
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
    rasterizer_state_info.cullMode = material->double_sided() ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
    rasterizer_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_state_info.depthBiasEnable = VK_FALSE;
    rasterizer_state_info.depthBiasConstantFactor = -4.75f;
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
    depth_stencil_state_info.depthWriteEnable = material->translucent() ? VK_FALSE : VK_TRUE;
    depth_stencil_state_info.depthCompareOp = material->translucent() ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_EQUAL;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (material->translucent()) {
        color_blend_attachment_state_info.blendEnable = VK_TRUE;
        color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        color_blend_attachment_state_info.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_infos[]{
        color_blend_attachment_state_info,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.logicOpEnable = VK_FALSE;
    color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = color_blend_attachment_state_infos;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = forward_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    struct specialization_data_t {
        int albedo_map{ -1 };
        int normal_map{ -1 };
        int roughness_map{ -1 };
        int metallic_map{ -1 };
        int emissive_map{ -1 };
        int ambient_map{ -1 };
        int max_maps{ -1 };
        int max_shadow_map_cascades{ graphics_limits::max_shadow_cascades };
        int translucent{ 0 };
    } specialization_data;

    const auto flags = material->flags();

    int index{ 0 };
    if (flags & material_flags::albedo_map_bit) {
        specialization_data.albedo_map = index++;
    }
    if (flags & material_flags::normal_map_bit) {
        specialization_data.normal_map = index++;
    }
    if (flags & material_flags::roughness_map_bit) {
        specialization_data.roughness_map = index++;
    }
    if (flags & material_flags::metallic_map_bit) {
        specialization_data.metallic_map = index++;
    }
    if (flags & material_flags::emissive_map_bit) {
        specialization_data.emissive_map = index++;
    }
    if (flags & material_flags::ambient_map_bit) {
        specialization_data.ambient_map = index++;
    }
    specialization_data.max_maps = index;

    if (flags & material_flags::translucent_bit) {
        specialization_data.translucent = 1;
    }

    VkSpecializationMapEntry specializtion_map_entries[9]{
        { 0, offsetof(specialization_data_t, albedo_map), sizeof(int) },
        { 1, offsetof(specialization_data_t, normal_map), sizeof(int) },
        { 2, offsetof(specialization_data_t, roughness_map), sizeof(int) },
        { 3, offsetof(specialization_data_t, metallic_map), sizeof(int) },
        { 4, offsetof(specialization_data_t, emissive_map), sizeof(int) },
        { 5, offsetof(specialization_data_t, ambient_map), sizeof(int) },
        { 6, offsetof(specialization_data_t, max_maps), sizeof(int) },
        { 7, offsetof(specialization_data_t, max_shadow_map_cascades), sizeof(int) },
        { 8, offsetof(specialization_data_t, translucent), sizeof(int) }
    };

    VkSpecializationInfo specialization_info;
    specialization_info.dataSize = sizeof(specialization_data);
    specialization_info.mapEntryCount = 9;
    specialization_info.pMapEntries = specializtion_map_entries;
    specialization_info.pData = &specialization_data;

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = forward_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    if (material->flags()) {
        fragment_shader_stage_info.pSpecializationInfo = &specialization_info;
    }

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.renderPass = _forward_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;

    VkPipeline pipeline;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, forward_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, forward_shader_modules[0], nullptr);

    return pipeline;
}

VkPipeline graphics_vulkan::_get_forward_pipeline(const std::shared_ptr<material>& material) {
    const auto flags = material ? material->flags() : 0;
    auto& pipeline = _forward_pipelines[flags];
    if (pipeline) {
        return pipeline;
    }
    return pipeline = _create_forward_pipeline(material);
}

void graphics_vulkan::_create_forward() {
    VkDescriptorSetLayoutBinding bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 1;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_forward_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const auto depth_format = _get_supported_depth_format();

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference resolve_reference{};
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

    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_forward_render_pass),
        "Failed to create render pass.");
}

void graphics_vulkan::_create_postprocess() {
    VkDescriptorSetLayoutBinding bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 1;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_postprocess_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_postprocess_render_pass),
        "Failed to create Vulkan render pass.");
}

void graphics_vulkan::_create_ssao_pipeline() {
    VkBufferCreateInfo ssao_buffer_info;
    ssao_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ssao_buffer_info.pNext = nullptr;
    ssao_buffer_info.flags = 0;
    ssao_buffer_info.size = sizeof(ssao_data);
    ssao_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    ssao_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ssao_buffer_info.queueFamilyIndexCount = 0;
    ssao_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo ssao_allocation_info{};
    ssao_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(_allocator, &ssao_buffer_info, &ssao_allocation_info, &_ssao_buffer, &_ssao_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    std::random_device random;
    std::mt19937 generator{ random() };
    std::uniform_real_distribution<float> distribution2{ 0.0f, 1.0f };
    std::uniform_real_distribution<float> distribution3{ 0.2f, 1.0f };

    ssao_data data;
    for (auto i = 0u; i < 64u; ++i) {
        data.samples[i] = {
            distribution2(generator) * 2.0f - 1.0f,
            distribution2(generator) * 2.0f - 1.0f,
            distribution2(generator)
        };

        auto scale = i / 64.0f;
        scale *= scale;

        // scale = 0.1f + scale * 0.9f;

        data.samples[i] = normalize(data.samples[i]) * distribution3(generator) * scale;
        //  data.samples[i] = normalize(data.samples[i]) * distribution2(generator) * scale;
    }

    void* ptr;
    vmaMapMemory(_allocator, _ssao_allocation, &ptr);
    std::memcpy(ptr, &data, sizeof(data));
    vmaUnmapMemory(_allocator, _ssao_allocation);

    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R32G32_SFLOAT;
    image_info.extent = { 4, 4, 1 };
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
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_ssao_noise_map, &_ssao_noise_map_allocation, nullptr),
        "Failed to create Vulkan image");

    vec2f ssao_noise[4 * 4];
    for (auto& noise : ssao_noise) {
        noise = {
            distribution2(generator) * 2.0f - 1.0f,
            distribution2(generator) * 2.0f - 1.0f
        };
    }

    // Create staging buffer.
    VkBufferCreateInfo buffer_info;
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = nullptr;
    buffer_info.flags = 0;
    buffer_info.size = sizeof(ssao_noise);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo staging_allocation_info{};
    staging_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_allocation;

    RB_VK(vmaCreateBuffer(_allocator, &buffer_info, &staging_allocation_info, &staging_buffer, &staging_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer");

    // Transfer pixels into buffer.
    RB_VK(vmaMapMemory(_allocator, staging_buffer_allocation, &ptr), "Failed to map staging buffer memory");
    std::memcpy(ptr, ssao_noise, buffer_info.size);
    vmaUnmapMemory(_allocator, staging_buffer_allocation);

    auto command_buffer = utils_vulkan::begin_single_time_commands(_device, _command_pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _ssao_noise_map;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { 4, 4, 1 };

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, _ssao_noise_map, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _ssao_noise_map;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    utils_vulkan::end_single_time_commands(_device, _graphics_queue, _command_pool, command_buffer);

    vmaDestroyBuffer(_allocator, staging_buffer, staging_buffer_allocation);

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _ssao_noise_map;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R32G32_SFLOAT;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_ssao_noise_map_view), "Failed to create image view");

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_ssao_sampler), "Failed to create Vulkan sampler");

    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8_UNORM;
    image_info.extent = { _swapchain_extent.width / (int)graphics_limits::ssao_image_reduction, _swapchain_extent.height / (int)graphics_limits::ssao_image_reduction, 1 };
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

    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_ssao_image, &_ssao_image_allocation, nullptr),
        "Failed to create Vulkan image");

    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _ssao_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_ssao_image_view), "Failed to create image view");

    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8_UNORM;
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

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_ssao_render_pass),
        "Failed to create Vulkan render pass.");

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = _ssao_render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &_ssao_image_view;
    framebuffer_info.width = _swapchain_extent.width / graphics_limits::ssao_image_reduction;
    framebuffer_info.height = _swapchain_extent.height / graphics_limits::ssao_image_reduction;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_ssao_framebuffer),
        "Failed to create Vulkan framebuffer");

    VkDescriptorSetLayoutBinding bindings[2]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 2;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_ssao_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorSetLayoutBinding blur_bindings[2]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 1;
    descriptor_set_layout_info.pBindings = blur_bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_ssao_blur_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 2;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_ssao_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _ssao_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_ssao_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_ssao_descriptor_set),
        "Failed to allocatore desctiptor set");

    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _ssao_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_ssao_blur_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_ssao_blur_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorBufferInfo buffer_infos[1]{
        { _ssao_buffer, 0, sizeof(ssao_data) },
    };

    VkDescriptorImageInfo image_infos[2]{
        { _ssao_sampler, _ssao_noise_map_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _ssao_sampler, _ssao_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[3]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _ssao_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[0], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _ssao_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _ssao_blur_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[1], nullptr, nullptr }
    };

    vkUpdateDescriptorSets(_device, 3, write_infos, 0, nullptr);

    VkDescriptorSetLayout layouts[4]{
        _main_descriptor_set_layout,
        _depth_descriptor_set_layout,
        _ssao_descriptor_set_layout,
        _postprocess_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 4;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_ssao_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule ssao_shader_modules[2];

    VkShaderModuleCreateInfo ssao_vert_shader_module_info;
    ssao_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ssao_vert_shader_module_info.pNext = nullptr;
    ssao_vert_shader_module_info.flags = 0;
    ssao_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    ssao_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &ssao_vert_shader_module_info, nullptr, &ssao_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo ssao_frag_shader_module_info;
    ssao_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ssao_frag_shader_module_info.pNext = nullptr;
    ssao_frag_shader_module_info.flags = 0;
    ssao_frag_shader_module_info.codeSize = shaders_vulkan::ssao_frag().size_bytes();
    ssao_frag_shader_module_info.pCode = shaders_vulkan::ssao_frag().data();
    RB_VK(vkCreateShaderModule(_device, &ssao_frag_shader_module_info, nullptr, &ssao_shader_modules[1]),
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
    viewport.width = static_cast<float>(_swapchain_extent.width / graphics_limits::ssao_image_reduction);
    viewport.height = static_cast<float>(_swapchain_extent.height / graphics_limits::ssao_image_reduction);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { _swapchain_extent.width / (int)graphics_limits::ssao_image_reduction, _swapchain_extent.height / (int)graphics_limits::ssao_image_reduction };

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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = ssao_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = ssao_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _ssao_pipeline_layout;
    pipeline_info.renderPass = _ssao_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_ssao_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, ssao_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, ssao_shader_modules[0], nullptr);

    VkDescriptorSetLayout ssao_blur_layouts[2]{
        _postprocess_descriptor_set_layout,
        _ssao_blur_descriptor_set_layout
    };

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(blur_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = ssao_blur_layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_ssao_blur_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule ssao_blur_shader_modules[2];

    VkShaderModuleCreateInfo ssao_blur_vert_shader_module_info;
    ssao_blur_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ssao_blur_vert_shader_module_info.pNext = nullptr;
    ssao_blur_vert_shader_module_info.flags = 0;
    ssao_blur_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    ssao_blur_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &ssao_blur_vert_shader_module_info, nullptr, &ssao_blur_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo ssao_blur_frag_shader_module_info;
    ssao_blur_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ssao_blur_frag_shader_module_info.pNext = nullptr;
    ssao_blur_frag_shader_module_info.flags = 0;
    ssao_blur_frag_shader_module_info.codeSize = shaders_vulkan::ssao_blur_frag().size_bytes();
    ssao_blur_frag_shader_module_info.pCode = shaders_vulkan::ssao_blur_frag().data();
    RB_VK(vkCreateShaderModule(_device, &ssao_blur_frag_shader_module_info, nullptr, &ssao_blur_shader_modules[1]),
        "Failed to create shader module");

    viewport.width = static_cast<float>(_swapchain_extent.width);
    viewport.height = static_cast<float>(_swapchain_extent.height);

    scissor.extent = _swapchain_extent;

    vertex_shader_stage_info.module = ssao_blur_shader_modules[0];
    fragment_shader_stage_info.module = ssao_blur_shader_modules[1];

    VkPipelineShaderStageCreateInfo ssao_blur_shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    pipeline_info.pStages = ssao_blur_shader_stages;
    pipeline_info.layout = _ssao_blur_pipeline_layout;
    pipeline_info.renderPass = _postprocess_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_ssao_blur_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, ssao_blur_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, ssao_blur_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_fxaa_pipeline() {
    VkDescriptorSetLayout layouts[1]{
        _postprocess_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_fxaa_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule fxaa_shader_modules[2];

    VkShaderModuleCreateInfo fxaa_vert_shader_module_info;
    fxaa_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fxaa_vert_shader_module_info.pNext = nullptr;
    fxaa_vert_shader_module_info.flags = 0;
    fxaa_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    fxaa_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &fxaa_vert_shader_module_info, nullptr, &fxaa_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fxaa_frag_shader_module_info;
    fxaa_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fxaa_frag_shader_module_info.pNext = nullptr;
    fxaa_frag_shader_module_info.flags = 0;
    fxaa_frag_shader_module_info.codeSize = shaders_vulkan::fxaa_frag().size_bytes();
    fxaa_frag_shader_module_info.pCode = shaders_vulkan::fxaa_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fxaa_frag_shader_module_info, nullptr, &fxaa_shader_modules[1]),
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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = fxaa_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = fxaa_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _fxaa_pipeline_layout;
    pipeline_info.renderPass = _postprocess_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_fxaa_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, fxaa_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, fxaa_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_blur_pipeline() {
    VkDescriptorSetLayout layouts[1]{
        _postprocess_descriptor_set_layout
    };

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(blur_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_blur_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule blur_shader_modules[2];

    VkShaderModuleCreateInfo blur_vert_shader_module_info;
    blur_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    blur_vert_shader_module_info.pNext = nullptr;
    blur_vert_shader_module_info.flags = 0;
    blur_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    blur_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &blur_vert_shader_module_info, nullptr, &blur_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo blur_frag_shader_module_info;
    blur_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    blur_frag_shader_module_info.pNext = nullptr;
    blur_frag_shader_module_info.flags = 0;
    blur_frag_shader_module_info.codeSize = shaders_vulkan::blur_frag().size_bytes();
    blur_frag_shader_module_info.pCode = shaders_vulkan::blur_frag().data();
    RB_VK(vkCreateShaderModule(_device, &blur_frag_shader_module_info, nullptr, &blur_shader_modules[1]),
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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = blur_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = blur_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _blur_pipeline_layout;
    pipeline_info.renderPass = _postprocess_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_blur_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, blur_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, blur_shader_modules[0], nullptr);
}


void graphics_vulkan::_create_sharpen_pipeline() {
    VkDescriptorSetLayout layouts[1]{
        _postprocess_descriptor_set_layout
    };

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(sharpen_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_sharpen_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule sharpen_shader_modules[2];

    VkShaderModuleCreateInfo sharpen_vert_shader_module_info;
    sharpen_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sharpen_vert_shader_module_info.pNext = nullptr;
    sharpen_vert_shader_module_info.flags = 0;
    sharpen_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    sharpen_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &sharpen_vert_shader_module_info, nullptr, &sharpen_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo sharpen_frag_shader_module_info;
    sharpen_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sharpen_frag_shader_module_info.pNext = nullptr;
    sharpen_frag_shader_module_info.flags = 0;
    sharpen_frag_shader_module_info.codeSize = shaders_vulkan::sharpen_frag().size_bytes();
    sharpen_frag_shader_module_info.pCode = shaders_vulkan::sharpen_frag().data();
    RB_VK(vkCreateShaderModule(_device, &sharpen_frag_shader_module_info, nullptr, &sharpen_shader_modules[1]),
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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = sharpen_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = sharpen_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _sharpen_pipeline_layout;
    pipeline_info.renderPass = _postprocess_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_sharpen_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, sharpen_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, sharpen_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_motion_blur_pipeline() {
    VkDescriptorSetLayout layouts[3]{
        _main_descriptor_set_layout,
        _depth_descriptor_set_layout,
        _postprocess_descriptor_set_layout,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 3;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_motion_blur_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule motion_blur_shader_modules[2];

    VkShaderModuleCreateInfo motion_blur_vert_shader_module_info;
    motion_blur_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    motion_blur_vert_shader_module_info.pNext = nullptr;
    motion_blur_vert_shader_module_info.flags = 0;
    motion_blur_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    motion_blur_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &motion_blur_vert_shader_module_info, nullptr, &motion_blur_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo motion_blur_frag_shader_module_info;
    motion_blur_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    motion_blur_frag_shader_module_info.pNext = nullptr;
    motion_blur_frag_shader_module_info.flags = 0;
    motion_blur_frag_shader_module_info.codeSize = shaders_vulkan::motion_blur_frag().size_bytes();
    motion_blur_frag_shader_module_info.pCode = shaders_vulkan::motion_blur_frag().data();
    RB_VK(vkCreateShaderModule(_device, &motion_blur_frag_shader_module_info, nullptr, &motion_blur_shader_modules[1]),
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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = motion_blur_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = motion_blur_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _motion_blur_pipeline_layout;
    pipeline_info.renderPass = _postprocess_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_motion_blur_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, motion_blur_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, motion_blur_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_fill_pipeline() {
    VkAttachmentDescription color_attachment;
    color_attachment.flags = 0;
    color_attachment.format = VK_FORMAT_R8_UNORM;
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

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass_desc;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();
    RB_VK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_fill_render_pass),
        "Failed to create Vulkan render pass.");

    VkDescriptorSetLayout layouts[1]{
        _main_descriptor_set_layout
    };

    VkPushConstantRange push_constant_range;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(local_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_fill_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule fill_shader_modules[2];

    VkShaderModuleCreateInfo fill_vert_shader_module_info;
    fill_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fill_vert_shader_module_info.pNext = nullptr;
    fill_vert_shader_module_info.flags = 0;
    fill_vert_shader_module_info.codeSize = shaders_vulkan::fill_vert().size_bytes();
    fill_vert_shader_module_info.pCode = shaders_vulkan::fill_vert().data();
    RB_VK(vkCreateShaderModule(_device, &fill_vert_shader_module_info, nullptr, &fill_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fill_frag_shader_module_info;
    fill_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fill_frag_shader_module_info.pNext = nullptr;
    fill_frag_shader_module_info.flags = 0;
    fill_frag_shader_module_info.codeSize = shaders_vulkan::fill_frag().size_bytes();
    fill_frag_shader_module_info.pCode = shaders_vulkan::fill_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fill_frag_shader_module_info, nullptr, &fill_shader_modules[1]),
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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = fill_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = fill_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _fill_pipeline_layout;
    pipeline_info.renderPass = _fill_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_fill_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, fill_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, fill_shader_modules[0], nullptr);

    VkDescriptorSetLayoutBinding bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info;
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pNext = nullptr;
    descriptor_set_layout_info.flags = 0;
    descriptor_set_layout_info.bindingCount = 1;
    descriptor_set_layout_info.pBindings = bindings;
    RB_VK(vkCreateDescriptorSetLayout(_device, &descriptor_set_layout_info, nullptr, &_fill_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");
}

void graphics_vulkan::_create_outline_pipeline() {
    VkDescriptorSetLayout layouts[2]{
        _fill_descriptor_set_layout,
        _postprocess_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 2;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_outline_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule outline_shader_modules[2];

    VkShaderModuleCreateInfo outline_vert_shader_module_info;
    outline_vert_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    outline_vert_shader_module_info.pNext = nullptr;
    outline_vert_shader_module_info.flags = 0;
    outline_vert_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    outline_vert_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &outline_vert_shader_module_info, nullptr, &outline_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo outline_frag_shader_module_info;
    outline_frag_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    outline_frag_shader_module_info.pNext = nullptr;
    outline_frag_shader_module_info.flags = 0;
    outline_frag_shader_module_info.codeSize = shaders_vulkan::outline_frag().size_bytes();
    outline_frag_shader_module_info.pCode = shaders_vulkan::outline_frag().data();
    RB_VK(vkCreateShaderModule(_device, &outline_frag_shader_module_info, nullptr, &outline_shader_modules[1]),
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
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state_info.alphaBlendOp = VK_BLEND_OP_ADD;

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
    vertex_shader_stage_info.module = outline_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = outline_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _outline_pipeline_layout;
    pipeline_info.renderPass = _postprocess_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_outline_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, outline_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, outline_shader_modules[0], nullptr);
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

    RB_VK(vkCreateSemaphore(_device, &semaphore_info, VK_NULL_HANDLE, &_render_semaphore), "Failed to create render semaphore");
    RB_VK(vkCreateSemaphore(_device, &semaphore_info, VK_NULL_HANDLE, &_present_semaphore), "Failed to create present semaphore");
    RB_VK(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _present_semaphore, VK_NULL_HANDLE, &_image_index),
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
    vertex_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
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
    index_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
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
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
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
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
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
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
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
            data.roughness = i / 5.0f;
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
    const auto depth_format = _get_supported_shadow_format();

    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = depth_format;
    image_info.extent = { graphics_limits::shadow_map_size, graphics_limits::shadow_map_size, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = graphics_limits::max_shadow_cascades;
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
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    image_view_info.format = depth_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = graphics_limits::max_shadow_cascades;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_shadow_image_view),
        "Failed to create Vulkan image view");

    for (auto i = 0u; i < graphics_limits::max_shadow_cascades; ++i) {
        VkImageViewCreateInfo image_view_info;
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = _shadow_image;
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = depth_format;
        image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = i;
        image_view_info.subresourceRange.layerCount = 1;
        RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_shadow_image_views[i]),
            "Failed to create Vulkan image view");
    }

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_TRUE;
    sampler_info.compareOp = VK_COMPARE_OP_LESS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_shadow_sampler), "Failed to create Vulkan sampler");

    VkAttachmentDescription attachment_description{};
    attachment_description.format = depth_format;
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

    for (auto i = 0u; i < graphics_limits::max_shadow_cascades; ++i) {
        VkFramebufferCreateInfo framebuffer_info;
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.pNext = nullptr;
        framebuffer_info.flags = 0;
        framebuffer_info.renderPass = _shadow_render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = &_shadow_image_views[i];
        framebuffer_info.width = graphics_limits::shadow_map_size;
        framebuffer_info.height = graphics_limits::shadow_map_size;
        framebuffer_info.layers = 1;
        RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_shadow_framebuffers[i]),
            "Failed to create Vulkan framebuffer");
    }

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
    rasterizer_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
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
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = nullptr;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_shadow_pipeline),
        "Failed to create Vulkan graphics pipeline");
}

void graphics_vulkan::_create_skybox_pipeline() {
    VkDescriptorSetLayout layouts[]{
        _main_descriptor_set_layout,
        _environment_descriptor_set_layout
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

    VkShaderModule skybox_shader_modules[2];

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::skybox_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::skybox_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &skybox_shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::skybox_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::skybox_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &skybox_shader_modules[1]),
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
    vertex_shader_stage_info.module = skybox_shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = skybox_shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.renderPass = _forward_render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_skybox_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, skybox_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, skybox_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_present_pipeline() {
    VkDescriptorSetLayout layouts[]{
        _forward_descriptor_set_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_present_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule shader_modules[2];

    VkShaderModuleCreateInfo vertex_shader_module_info;
    vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_shader_module_info.pNext = nullptr;
    vertex_shader_module_info.flags = 0;
    vertex_shader_module_info.codeSize = shaders_vulkan::quad_vert().size_bytes();
    vertex_shader_module_info.pCode = shaders_vulkan::quad_vert().data();
    RB_VK(vkCreateShaderModule(_device, &vertex_shader_module_info, nullptr, &shader_modules[0]),
        "Failed to create shader module");

    VkShaderModuleCreateInfo fragment_shader_module_info;
    fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_shader_module_info.pNext = nullptr;
    fragment_shader_module_info.flags = 0;
    fragment_shader_module_info.codeSize = shaders_vulkan::present_frag().size_bytes();
    fragment_shader_module_info.pCode = shaders_vulkan::present_frag().data();
    RB_VK(vkCreateShaderModule(_device, &fragment_shader_module_info, nullptr, &shader_modules[1]),
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
    depth_stencil_state_info.depthTestEnable = VK_FALSE;
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
    vertex_shader_stage_info.module = shader_modules[0];
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = shader_modules[1];
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates = dynamic_states;

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
    pipeline_info.layout = _present_pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_present_pipeline),
        "Failed to create Vulkan graphics pipeline");

    pipeline_info.renderPass = _forward_render_pass;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_light_copy_pipeline),
        "Failed to create Vulkan graphics pipeline");

    pipeline_info.renderPass = _postprocess_render_pass;
    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_forward_copy_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, shader_modules[0], nullptr);
}

void graphics_vulkan::_create_command_buffers() {
    VkCommandBufferAllocateInfo command_buffer_alloc_info;
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = _command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = max_command_buffers;

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
    _command_index = (_command_index + 1) % max_command_buffers;

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

VkFormat graphics_vulkan::_get_supported_depth_format() {
    VkFormat depth_formats[]{
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depth_formats) {
        if (_is_format_supported(format, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat graphics_vulkan::_get_supported_shadow_format() {
    VkFormat depth_formats[]{
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM,
    };

    for (auto& format : depth_formats) {
        if (_is_format_supported(format, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool graphics_vulkan::_is_format_supported(VkFormat format, VkFormatFeatureFlags flags) {
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(_physical_device, format, &format_props);
    return (format_props.optimalTilingFeatures & flags) == flags;
}
