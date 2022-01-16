#include "graphics_vulkan.hpp"
#include "utils_vulkan.hpp"
#include "mesh_vulkan.hpp"

#include "shaders/forward.vert.generated.h"
#include "shaders/forward.frag.generated.h"
#include "shaders/culling.comp.generated.h"
#include "shaders/depth_pyramid.comp.generated.h"
#include "shaders/occlusion.vert.generated.h"

using namespace rb;

constexpr auto sphere_slices = 8u;
constexpr auto sphere_stacks = 8u;
constexpr auto sphere_vertex_count = (sphere_slices + 1) * (sphere_stacks + 1);
constexpr auto sphere_index_count = sphere_stacks * sphere_slices * 6;

graphics_vulkan::graphics_vulkan() {
    _create_main_buffer();
    _create_world_buffer();
    _create_vertex_buffer();
    _create_index_buffer();
    _create_draw_buffer();
    _create_forward_pipeline();
    _create_depth_pyramid();
    _create_sphere();
    _create_occlusion_query_pool();
    _create_occlusion_pipeline();
    _create_culling_pipeline();
}

graphics_vulkan::~graphics_vulkan() {
    for (auto& fence : _fences) {
        vkWaitForFences(_device, 1, &fence, VK_TRUE, 1000000000);
        vkDestroyFence(_device, fence, nullptr);
    }
    vkFreeCommandBuffers(_device, _command_pool, max_command_buffers, _command_buffers);

    vkDestroyPipeline(_device, _depth_pyramid_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _depth_pyramid_pipeline_layout, nullptr);
    vkDestroyDescriptorPool(_device, _depth_pyramid_descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(_device, _depth_pyramid_descriptor_set_layout, nullptr);
    vkDestroySampler(_device, _depth_pyramid_sampler, nullptr);
    for (auto i = 0u; i < _depth_pyramid_levels; ++i) {
        vkDestroyImageView(_device, _depth_pyramid_mips[i], nullptr);
    }
    vkDestroyImageView(_device, _depth_pyramid_view, nullptr);
    vmaDestroyImage(_allocator, _depth_pyramid, _depth_pyramid_allocation);

    vmaDestroyBuffer(_allocator, _sphere_index_buffer, _sphere_index_buffer_allocation);
    vmaDestroyBuffer(_allocator, _sphere_vertex_buffer, _sphere_vertex_buffer_allocation);

    vkDestroyPipeline(_device, _culling_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _culling_pipeline_layout, nullptr);
    vkDestroyDescriptorPool(_device, _culling_descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(_device, _culling_descriptor_set_layout, nullptr);

    vmaDestroyBuffer(_allocator, _occlusion_buffer, _occlusion_buffer_allocation);
    vkDestroyQueryPool(_device, _occlusion_query_pool, nullptr);

    vkDestroyPipeline(_device, _occlusion_pipeline, nullptr);

    vkDestroyPipeline(_device, _forward_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _forward_pipeline_layout, nullptr);
    vkDestroyDescriptorPool(_device, _forward_descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(_device, _forward_descriptor_set_layout, nullptr);

    vmaDestroyBuffer(_allocator, _draw_buffer, _draw_buffer_allocation);
    vmaDestroyBuffer(_allocator, _draw_output_buffer, _draw_output_buffer_allocation);
    vmaDestroyBuffer(_allocator, _index_buffer, _index_buffer_allocation);
    vmaDestroyBuffer(_allocator, _vertex_buffer, _vertex_buffer_allocation);
    vmaDestroyBuffer(_allocator, _world_buffer, _world_buffer_allocation);
    vmaDestroyBuffer(_allocator, _main_staging_buffer, _main_staging_buffer_allocation);
    vmaDestroyBuffer(_allocator, _main_buffer, _main_buffer_allocation);
}

std::shared_ptr<mesh> graphics_vulkan::make_mesh(const mesh_desc& desc) {
    const auto vertex_buffer_offset = _vertex_buffer_offset;
    const auto index_buffer_offset = _index_buffer_offset;

    if (!desc.vertices.empty()) {
        void* ptr;
        RB_VK(vmaMapMemory(_allocator, _vertex_buffer_allocation, &ptr), "Failed to map memory");
        std::memcpy(static_cast<vertex*>(ptr) + _vertex_buffer_offset, desc.vertices.data(), desc.vertices.size_bytes());
        vmaUnmapMemory(_allocator, _vertex_buffer_allocation);

        _vertex_buffer_offset += static_cast<std::uint32_t>(desc.vertices.size());
    }

    if (!desc.indices.empty()) {
        void* ptr;
        RB_VK(vmaMapMemory(_allocator, _index_buffer_allocation, &ptr), "Failed to map memory");
        std::memcpy(static_cast<std::uint32_t*>(ptr) + _index_buffer_offset, desc.indices.data(), desc.indices.size_bytes());
        vmaUnmapMemory(_allocator, _index_buffer_allocation);

        _index_buffer_offset += static_cast<std::uint32_t>(desc.indices.size());
    }

    return std::make_shared<mesh_vulkan>(vertex_buffer_offset, index_buffer_offset, desc);
}

void graphics_vulkan::set_camera(const mat4f& proj, const mat4f& view, const mat4f& world) {
    _main_staging_buffer_data.proj = proj;
    _main_staging_buffer_data.view = view;
    _main_staging_buffer_data.proj_view = proj * view;
    _main_staging_buffer_data.inv_proj_view = invert(_main_staging_buffer_data.proj_view);
    _main_staging_buffer_data.camera_position = { world[12], world[13], world[14], 0.0f };
}

void graphics_vulkan::render(const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material) {
    const auto native_mesh = std::static_pointer_cast<mesh_vulkan>(mesh);
    const auto& lod = native_mesh->lods().front();

    _world_buffer_staging.push_back({ world, mesh->bsphere()});
    _draw_buffer_staging.push_back({
       lod.size,
       1u,
       native_mesh->index_offset() + lod.offset,
       static_cast<std::int32_t>(native_mesh->vertex_offset()),
       _instance_index
    });

    ++_instance_index;
}

void graphics_vulkan::present() {
    const auto projection_t = transpose(_main_staging_buffer_data.proj);

    const auto a = *reinterpret_cast<const vec4f*>(&projection_t.values[3 * 4]);
    const auto b = *reinterpret_cast<const vec4f*>(&projection_t.values[0 * 4]);
    const auto c = *reinterpret_cast<const vec4f*>(&projection_t.values[1 * 4]);

    const auto frustum_x = normalize_plane(a + b); // x + w < 0
    const auto frustum_y = normalize_plane(a + c); // y + w < 0

    _main_staging_buffer_data.camera_frustum = { frustum_x.x, frustum_x.z, frustum_y.y, frustum_y.z };
    _main_staging_buffer_data.instance_count = _instance_index;
    _main_staging_buffer_data.total_frames = _total_frames;

    // Sorting 
    std::sort(_draw_buffer_staging.begin(), _draw_buffer_staging.begin() + _instance_index, [&](VkDrawIndexedIndirectCommand& lhs, VkDrawIndexedIndirectCommand& rhs) {
        vec3f camera_position{ _main_staging_buffer_data.camera_position.x, _main_staging_buffer_data.camera_position.y, _main_staging_buffer_data.camera_position.z };
        
        auto& lhs_transform = _world_buffer_staging[lhs.firstInstance].transform;
        auto& rhs_transform = _world_buffer_staging[rhs.firstInstance].transform;

        vec3f lhs_position{ lhs_transform[12], lhs_transform[13], lhs_transform[14] };
        vec3f rhs_position{ rhs_transform[12], rhs_transform[13], rhs_transform[14] };

        lhs_position = lhs_position + _world_buffer_staging[lhs.firstInstance].bsphere.position;
        rhs_position = rhs_position + _world_buffer_staging[rhs.firstInstance].bsphere.position;

        return length(lhs_position - camera_position) < length(rhs_position - camera_position);
    });

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _main_staging_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, &_main_staging_buffer_data, sizeof(main_data));
    vmaUnmapMemory(_allocator, _main_staging_buffer_allocation);

    RB_VK(vmaMapMemory(_allocator, _world_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, _world_buffer_staging.data(), sizeof(world_data)* _instance_index);
    vmaUnmapMemory(_allocator, _world_buffer_allocation);

    RB_VK(vmaMapMemory(_allocator, _draw_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, _draw_buffer_staging.data(), sizeof(VkDrawIndexedIndirectCommand)* _instance_index);
    vmaUnmapMemory(_allocator, _draw_buffer_allocation);

    auto command_buffer = _command_begin();

    VkBufferCopy copy;
    copy.srcOffset = copy.dstOffset = 0;
    copy.size = sizeof(main_data);
    vkCmdCopyBuffer(command_buffer, _main_staging_buffer, _main_buffer, 1, &copy);

    /*if (_total_frames > 0) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _depth_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _depth_pyramid_pipeline);

        for (auto i = 0; i < _depth_pyramid_levels; ++i) {
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _depth_pyramid_pipeline_layout, 0, 1, &_depth_pyramid_descriptor_sets[i], 0, nullptr);

            vec2u depth_pyramid_size;
            depth_pyramid_size.x = std::max(window::size().x >> i, 1u);
            depth_pyramid_size.y = std::max(window::size().y >> i, 1u);

            vkCmdPushConstants(command_buffer, _depth_pyramid_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(depth_pyramid_size), &depth_pyramid_size);
            vkCmdDispatch(command_buffer, (depth_pyramid_size.x + 32 - 1) / 32, (depth_pyramid_size.y + 32 - 1) / 32, 1);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = _depth_pyramid;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = i;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _depth_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
    }*/

    VkClearValue clear_values[1];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    // clear_values[1].depthStencil = { 1.0f, 0 };

    const auto window_size = window::size();

    vkCmdResetQueryPool(command_buffer, _occlusion_query_pool, 0, _main_staging_buffer_data.instance_count);

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.framebuffer = _framebuffers[_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { window_size.x, window_size.y };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    // vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{ 0.0f, 0.0f, (float)window_size.x, (float)window_size.y, 0.0f, 1.0f };

    VkRect2D scissor{ { 0, 0 }, { window_size.x, window_size.y } };

    VkDeviceSize offset{ 0 };

    VkClearAttachment clear_attachments[2] = {};

    clear_attachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clear_attachments[0].clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clear_attachments[0].colorAttachment = 0;

    clear_attachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    clear_attachments[1].clearValue.depthStencil = { 1.0f, 0 };

    VkClearRect clear_rect = {};
    clear_rect.layerCount = 1;
    clear_rect.rect.offset = { 0, 0 };
    clear_rect.rect.extent = scissor.extent;

    if (_main_staging_buffer_data.total_frames > 0) {
        vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _occlusion_pipeline);
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &_sphere_vertex_buffer, &offset);
        vkCmdBindIndexBuffer(command_buffer, _sphere_index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _forward_pipeline_layout, 0, 1, &_forward_descriptor_set, 0, nullptr);

        //vkCmdBeginQuery(command_buffer, _occlusion_query_pool, 0, 0);
        for (auto i = 0u; i < _main_staging_buffer_data.instance_count; ++i) {
            auto id = _draw_buffer_staging[i].firstInstance;

            vkCmdBeginQuery(command_buffer, _occlusion_query_pool, id, 0);
            vkCmdDrawIndexed(command_buffer, sphere_index_count, 1, 0, 0, id);
            vkCmdEndQuery(command_buffer, _occlusion_query_pool, id);
        }

        //vkCmdCopyQueryPoolResults(command_buffer, _occlusion_query_result, 0, _main_staging_buffer_data.instance_count, )

        vkCmdEndRenderPass(command_buffer);

        vkCmdCopyQueryPoolResults(command_buffer, _occlusion_query_pool, 0, _main_staging_buffer_data.instance_count,
            _occlusion_buffer, 0u, sizeof(std::uint32_t), VK_QUERY_RESULT_WAIT_BIT);

        // _command_end();

        //if (_main_staging_buffer_data.total_frames > 0) {
        //    vkGetQueryPoolResults(
        //        _device,
        //        _occlusion_query_pool,
        //        0,
        //        _main_staging_buffer_data.instance_count,
        //        sizeof(uint32_t) * _main_staging_buffer_data.instance_count,
        //        _occlusion_query_result.get(),
        //        sizeof(uint32_t),
        //        // Store results a 64 bit values and wait until the results have been finished
        //        // If you don't want to wait, you can use VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
        //        // which also returns the state of the result (ready) in the result
        //        VK_QUERY_RESULT_WAIT_BIT);

        //    // window::set_title(format("Passed samples: {}", samples));
        //}

        // command_buffer = _command_begin();

    }

    VkDescriptorSet culling_descriptor_sets[]{
        _forward_descriptor_set,
        _culling_descriptor_set,
    };
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _culling_pipeline_layout, 0, 2, culling_descriptor_sets, 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _culling_pipeline);

    _buffer_barrier(command_buffer, _draw_output_buffer, 0, sizeof(VkDrawIndexedIndirectCommand) * _instance_index,
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    vkCmdDispatch(command_buffer, (_instance_index + _instance_index % 16) / 16, 1, 1);

    _buffer_barrier(command_buffer, _draw_output_buffer, 0, sizeof(VkDrawIndexedIndirectCommand) * _instance_index,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdClearAttachments(command_buffer, 2, clear_attachments, 1, &clear_rect);

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffer, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _forward_pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _forward_pipeline_layout, 0, 1, &_forward_descriptor_set, 0, nullptr);

    //for (auto i = 0u; i < _main_staging_buffer_data.instance_count; ++i) {
    //    auto& cmd = _draw_buffer_staging[i];
    //    auto id = _draw_buffer_staging[i].firstInstance;
    //    if (_main_staging_buffer_data.total_frames == 0 || _occlusion_query_result[id] > 0) {
    //        vkCmdDrawIndexed(command_buffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
    //    }
    //}

    vkCmdDrawIndexedIndirect(command_buffer, _draw_output_buffer, 0, _instance_index, sizeof(VkDrawIndexedIndirectCommand));

    vkCmdEndRenderPass(command_buffer);

    _command_end();

    graphics_base_vulkan::present();

    _world_buffer_staging.clear();
    _draw_buffer_staging.clear();
    _instance_index = 0;
    ++_total_frames;
}

void graphics_vulkan::_create_main_buffer() {
    _create_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        sizeof(main_data),
        &_main_staging_buffer,
        &_main_staging_buffer_allocation);

    _create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        sizeof(main_data),
        &_main_buffer,
        &_main_buffer_allocation);
}

void graphics_vulkan::_create_world_buffer() {
    _create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        sizeof(world_data) * max_world_size,
        &_world_buffer,
        &_world_buffer_allocation);
}

void graphics_vulkan::_create_vertex_buffer() {
    _create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        max_vertex_size_in_bytes / sizeof(vertex),
        &_vertex_buffer,
        &_vertex_buffer_allocation);
}

void graphics_vulkan::_create_index_buffer() {
    _create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        max_index_size_in_bytes / sizeof(std::uint32_t),
        &_index_buffer,
        &_index_buffer_allocation);
}

void graphics_vulkan::_create_draw_buffer() {
    _create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        sizeof(VkDrawIndexedIndirectCommand) * max_world_size,
        &_draw_buffer,
        &_draw_buffer_allocation);

    _create_buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        sizeof(VkDrawIndexedIndirectCommand) * max_world_size,
        &_draw_output_buffer,
        &_draw_output_buffer_allocation);
}

void graphics_vulkan::_create_forward_pipeline() {
    VkDescriptorSetLayoutBinding bindings[3]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };
    _create_descriptor_set_layout(bindings, 0, &_forward_descriptor_set_layout);

    VkDescriptorPoolSize pool_sizes[3]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 2;
    descriptor_pool_info.poolSizeCount = 3;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_forward_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _forward_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_forward_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_forward_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorBufferInfo buffer_infos[2]{
        { _main_buffer, 0, sizeof(main_data) },
        { _world_buffer, 0, sizeof(world_data) * max_world_size }
    };

    VkWriteDescriptorSet write_infos[2]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[0], nullptr},
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &buffer_infos[1], nullptr },
    };

    vkUpdateDescriptorSets(_device, 2, write_infos, 0, nullptr);

    VkDescriptorSetLayout layouts[1]{
        _forward_descriptor_set_layout,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = sizeof(layouts) / sizeof(*layouts);
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_forward_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule forward_shader_modules[2];
    _create_shader_module_from_code(shader_stage::vertex, forward_vert, {}, &forward_shader_modules[0]);
    _create_shader_module_from_code(shader_stage::fragment, forward_frag, {}, &forward_shader_modules[1]);

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(vertex);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[3]{
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position) },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, texcoord) },
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, normal) }
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

    //color_blend_attachment_state_info.blendEnable = VK_TRUE;
    //color_blend_attachment_state_info.colorBlendOp = VK_BLEND_OP_ADD;
    //color_blend_attachment_state_info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    //color_blend_attachment_state_info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

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

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = forward_shader_modules[1];
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
    pipeline_info.layout = _forward_pipeline_layout;
    pipeline_info.renderPass = _render_pass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;

    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_forward_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, forward_shader_modules[1], nullptr);
    vkDestroyShaderModule(_device, forward_shader_modules[0], nullptr);
}

void graphics_vulkan::_create_culling_pipeline() {
    VkDescriptorSetLayoutBinding bindings[4]{
        { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    _create_descriptor_set_layout(bindings, 0, &_culling_descriptor_set_layout);

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_culling_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _culling_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_culling_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_culling_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorBufferInfo buffer_infos[3]{
        { _draw_buffer, 0, sizeof(VkDrawIndexedIndirectCommand) * max_world_size },
        { _draw_output_buffer, 0, sizeof(VkDrawIndexedIndirectCommand) * max_world_size },
        { _occlusion_buffer, 0, sizeof(std::uint32_t) * max_world_size }
    };

    VkDescriptorImageInfo image_infos[1]{
        { _depth_pyramid_sampler, _depth_pyramid_view, VK_IMAGE_LAYOUT_GENERAL },
    };

    VkWriteDescriptorSet write_infos[4]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _culling_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &buffer_infos[0], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _culling_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &buffer_infos[1], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _culling_descriptor_set, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _culling_descriptor_set, 3, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &buffer_infos[2], nullptr },
    };

    vkUpdateDescriptorSets(_device, 4, write_infos, 0, nullptr);

    VkDescriptorSetLayout layouts[2]{
        _forward_descriptor_set_layout,
        _culling_descriptor_set_layout,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = sizeof(layouts) / sizeof(*layouts);
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_culling_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule culling_shader_module;
    _create_shader_module_from_code(shader_stage::compute, culling_comp, {}, &culling_shader_module);

    _create_compute_pipeline(culling_shader_module, _culling_pipeline_layout, &_culling_pipeline);

    vkDestroyShaderModule(_device, culling_shader_module, nullptr);
}

void graphics_vulkan::_create_sphere() {
    auto vertices = std::make_unique<vec3f[]>(sphere_vertex_count);
    auto indices = std::make_unique<std::uint16_t[]>(sphere_index_count);

    float phi, theta;
	constexpr float dphi = pi<float>() / sphere_stacks;
    constexpr float dtheta = (2.0f * pi<float>()) / sphere_slices;
    float x, y, z, sc;
    unsigned int index = 0, stack, slice;
    int k;

    for (stack = 0; stack <= sphere_stacks; stack++) {
        phi = pi<float>() * 0.5f - stack * dphi;
		y = std::sin(phi);
		sc = -std::cos(phi);

		for (slice = 0; slice <= sphere_slices; slice++) {
			theta = slice * dtheta;
			x = sc * std::sin(theta);
			z = sc * std::cos(theta);

			vertices[index] = { x, y, z };

			index++;
		}
	}

	index = 0;
	k = sphere_slices + 1;

	for (stack = 0; stack < sphere_stacks; stack++) {
		for (slice = 0; slice < sphere_slices; slice++) {
			indices[index++] = (stack + 0) * k + slice;
			indices[index++] = (stack + 1) * k + slice;
			indices[index++] = (stack + 0) * k + slice + 1;

			indices[index++] = (stack + 0) * k + slice + 1;
			indices[index++] = (stack + 1) * k + slice;
			indices[index++] = (stack + 1) * k + slice + 1;
		}
	}

    _create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        sizeof(vec3f) * sphere_vertex_count,
        &_sphere_vertex_buffer,
        &_sphere_vertex_buffer_allocation);

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _sphere_vertex_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, vertices.get(), sizeof(vec3f) * sphere_vertex_count);
    vmaUnmapMemory(_allocator, _sphere_vertex_buffer_allocation);

    _create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        sizeof(std::uint16_t) * sphere_index_count,
        &_sphere_index_buffer,
        &_sphere_index_buffer_allocation);

    RB_VK(vmaMapMemory(_allocator, _sphere_index_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, indices.get(), sizeof(std::uint16_t) * sphere_index_count);
    vmaUnmapMemory(_allocator, _sphere_index_buffer_allocation);
}

void graphics_vulkan::_create_occlusion_query_pool() {
    VkQueryPoolCreateInfo query_pool_info{};
    query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.queryType = VK_QUERY_TYPE_OCCLUSION;
    query_pool_info.queryCount = max_world_size;
    RB_VK(vkCreateQueryPool(_device, &query_pool_info, NULL, &_occlusion_query_pool), "Failed to create occlusion query pool");

    _occlusion_query_result = std::make_unique<std::uint32_t[]>(max_world_size);

    _create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        sizeof(std::uint32_t) * max_world_size,
        &_occlusion_buffer,
        &_occlusion_buffer_allocation);
}

void graphics_vulkan::_create_occlusion_pipeline() {
    VkShaderModule occlusion_shader_module;
    _create_shader_module_from_code(shader_stage::vertex, occlusion_vert, {}, &occlusion_shader_module);

    VkVertexInputBindingDescription vertex_input_binding_desc;
    vertex_input_binding_desc.binding = 0;
    vertex_input_binding_desc.stride = sizeof(vec3f);
    vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attributes[1]{
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
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
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_FALSE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_info{};
    color_blend_attachment_state_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state_info.blendEnable = VK_FALSE;

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
    vertex_shader_stage_info.module = occlusion_shader_module;
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
    };

    VkDynamicState dynamic_states[]{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info;
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pNext = nullptr;
    dynamic_state_info.flags = 0;
    dynamic_state_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(*dynamic_states);
    dynamic_state_info.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = sizeof(shader_stages) / sizeof(*shader_stages);
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
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDynamicState = &dynamic_state_info;

    RB_VK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_occlusion_pipeline),
        "Failed to create Vulkan graphics pipeline");

    vkDestroyShaderModule(_device, occlusion_shader_module, nullptr);
}

void graphics_vulkan::_create_depth_pyramid() {
    _depth_pyramid_levels = std::min((std::uint32_t)std::floor(std::log2(std::max(window::size().x, window::size().y))) + 1u, 16u);

    _create_image(VK_IMAGE_TYPE_2D, VK_FORMAT_R32_SFLOAT, { window::size().x, window::size().y, 1 }, _depth_pyramid_levels, 1, 
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VMA_MEMORY_USAGE_GPU_ONLY, &_depth_pyramid, &_depth_pyramid_allocation);

    _create_image_view(_depth_pyramid, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_SFLOAT,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, _depth_pyramid_levels, 0, 1 }, &_depth_pyramid_view);

    for (auto i = 0u; i < _depth_pyramid_levels; ++i) {
        _create_image_view(_depth_pyramid, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_SFLOAT,
            { VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, 1 }, &_depth_pyramid_mips[i]);
    }

    _create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        1.0f, static_cast<float>(_depth_pyramid_levels), true, &_depth_pyramid_sampler);

    VkDescriptorSetLayoutBinding bindings[2]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    _create_descriptor_set_layout(bindings, 0, &_depth_pyramid_descriptor_set_layout);

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 16;
    descriptor_pool_info.poolSizeCount = 2;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_depth_pyramid_descriptor_pool),
        "Failed to create descriptor pool");

    for (auto i = 0u; i < _depth_pyramid_levels; ++i) {
        VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
        descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_allocate_info.pNext = nullptr;
        descriptor_set_allocate_info.descriptorPool = _depth_pyramid_descriptor_pool;
        descriptor_set_allocate_info.descriptorSetCount = 1;
        descriptor_set_allocate_info.pSetLayouts = &_depth_pyramid_descriptor_set_layout;
        RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_depth_pyramid_descriptor_sets[i]),
            "Failed to allocatore desctiptor set");

        VkDescriptorImageInfo image_infos[2]{
            { _depth_pyramid_sampler, i == 0 ? _depth_image_view : _depth_pyramid_mips[i - 1], i == 0 ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL },
            { _depth_pyramid_sampler, _depth_pyramid_mips[i], VK_IMAGE_LAYOUT_GENERAL},
        };

        VkWriteDescriptorSet write_infos[2]{
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _depth_pyramid_descriptor_sets[i], 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr},
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _depth_pyramid_descriptor_sets[i], 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &image_infos[1], nullptr, nullptr },
        };

        vkUpdateDescriptorSets(_device, 2, write_infos, 0, nullptr);
    }

    VkDescriptorSetLayout layouts[1]{
        _depth_pyramid_descriptor_set_layout,
    };

    VkPushConstantRange push_constant;
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset = 0;
    push_constant.size = sizeof(vec2u);

    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = sizeof(layouts) / sizeof(*layouts);
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant;
    RB_VK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_depth_pyramid_pipeline_layout),
        "Failed to create Vulkan pipeline layout");

    VkShaderModule depth_pyramid_shader_module;
    _create_shader_module_from_code(shader_stage::compute, depth_pyramid_comp, {}, &depth_pyramid_shader_module);

    _create_compute_pipeline(depth_pyramid_shader_module, _depth_pyramid_pipeline_layout, &_depth_pyramid_pipeline);

    vkDestroyShaderModule(_device, depth_pyramid_shader_module, nullptr);

    {
        VkDescriptorImageInfo image_infos[1]{
            { _depth_pyramid_sampler, _depth_pyramid_view, VK_IMAGE_LAYOUT_GENERAL },
        };

        VkWriteDescriptorSet write_infos[1]{
            { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        };

        vkUpdateDescriptorSets(_device, 1, write_infos, 0, nullptr);
    }
}
