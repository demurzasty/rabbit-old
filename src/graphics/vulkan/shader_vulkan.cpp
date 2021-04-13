#include "shader_vulkan.hpp"

#include <rabbit/core/exception.hpp>

using namespace rb;

shader_vulkan::shader_vulkan(VkDevice device, const shader_desc& desc)
	: shader(desc)
	, _device(device) {
	auto vertex_shader_module = create_shader_module(desc.vertex_bytecode);
	auto fragment_shader_module = create_shader_module(desc.fragment_bytecode);

	VkPipelineShaderStageCreateInfo vertex_shader_stage_info;
	vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_stage_info.pNext = nullptr;
	vertex_shader_stage_info.flags = 0;
	vertex_shader_stage_info.module = vertex_shader_module;
	vertex_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo fragment_shader_stage_info;
	fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_stage_info.pNext = nullptr;
	fragment_shader_stage_info.flags = 0;
	fragment_shader_stage_info.module = fragment_shader_module;
	fragment_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info, fragment_shader_stage_info };

	vkDestroyShaderModule(_device, fragment_shader_module, nullptr);
	vkDestroyShaderModule(_device, vertex_shader_module, nullptr);
}

shader_vulkan::~shader_vulkan() {
}

VkShaderModule shader_vulkan::create_shader_module(const span<const std::uint8_t>& bytecode) {
	VkShaderModuleCreateInfo shader_module_info;
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.pNext = nullptr;
	shader_module_info.flags = 0;
	shader_module_info.codeSize = bytecode.size();
	shader_module_info.pCode = reinterpret_cast<const std::uint32_t*>(bytecode.data());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(_device, &shader_module_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw make_exception("Failed to create shader module");
	}

	return shader_module;
}
