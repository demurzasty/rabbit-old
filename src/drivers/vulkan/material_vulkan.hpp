#pragma once 

#include <rabbit/engine/graphics/material.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
	class material_vulkan : public material {
		struct data {
			vec4f base_color;
			float roughness;
			float metallic;
		};

	public:
		material_vulkan(VkDevice device,
			VmaAllocator allocator,
			const material_desc& desc);

		~material_vulkan();

		VkDescriptorSetLayout descriptor_set_layout() const;

		VkDescriptorSet descriptor_set() const;

	private:
		VkDevice _device;
		VmaAllocator _allocator;

		VkBuffer _uniform_buffer;
		VmaAllocation _uniform_buffer_allocation;

		VkDescriptorSetLayout _descriptor_set_layout;
		VkDescriptorPool _descriptor_pool;
		VkDescriptorSet _descriptor_set;
	};
}
