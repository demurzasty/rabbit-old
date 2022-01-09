#include "graphics_vulkan.hpp"
#include "utils_vulkan.hpp"
#include "mesh_vulkan.hpp"

#include "shaders/forward.vert.generated.h"
#include "shaders/forward.frag.generated.h"
#include "shaders/culling.comp.generated.h"

using namespace rb;

graphics_vulkan::graphics_vulkan() {
    _create_main_buffer();
    _create_world_buffer();
    _create_vertex_buffer();
    _create_index_buffer();
    _create_draw_buffer();
    _create_forward_pipeline();
    _create_culling_pipeline();
}

graphics_vulkan::~graphics_vulkan() {
    for (auto& fence : _fences) {
        vkWaitForFences(_device, 1, &fence, VK_TRUE, 1000000000);
        vkDestroyFence(_device, fence, nullptr);
    }
    vkFreeCommandBuffers(_device, _command_pool, max_command_buffers, _command_buffers);

    vkDestroyPipeline(_device, _culling_pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _culling_pipeline_layout, nullptr);
    vkDestroyDescriptorPool(_device, _culling_descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(_device, _culling_descriptor_set_layout, nullptr);

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

    VkClearValue clear_values[2];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clear_values[1].depthStencil = { 1.0f, 0 };

    const auto window_size = window::size();

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _render_pass;
    render_pass_begin_info.framebuffer = _framebuffers[_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { window_size.x, window_size.y };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{ 0.0f, 0.0f, (float)window_size.x, (float)window_size.y, 0.0f, 1.0f };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{ { 0, 0 }, { window_size.x, window_size.y } };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffer, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _forward_pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _forward_pipeline_layout, 0, 1, &_forward_descriptor_set, 0, nullptr);

    vkCmdDrawIndexedIndirect(command_buffer, _draw_output_buffer, 0, _instance_index, sizeof(VkDrawIndexedIndirectCommand));

    vkCmdEndRenderPass(command_buffer);

    _command_end();

    graphics_base_vulkan::present();

    _world_buffer_staging.clear();
    _draw_buffer_staging.clear();
    _instance_index = 0;
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
    VkDescriptorSetLayoutBinding bindings[2]{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    _create_descriptor_set_layout(bindings, 0, &_forward_descriptor_set_layout);

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 2;
    descriptor_pool_info.poolSizeCount = 2;
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
    VkDescriptorSetLayoutBinding bindings[2]{
        { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };
    _create_descriptor_set_layout(bindings, 0, &_culling_descriptor_set_layout);

    VkDescriptorPoolSize pool_sizes[1]{
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 1;
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

    VkDescriptorBufferInfo buffer_infos[2]{
        { _draw_buffer, 0, sizeof(VkDrawIndexedIndirectCommand) * max_world_size },
        { _draw_output_buffer, 0, sizeof(VkDrawIndexedIndirectCommand) * max_world_size }
    };

    VkWriteDescriptorSet write_infos[2]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _culling_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &buffer_infos[0], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _culling_descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &buffer_infos[1], nullptr },
    };

    vkUpdateDescriptorSets(_device, 2, write_infos, 0, nullptr);

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