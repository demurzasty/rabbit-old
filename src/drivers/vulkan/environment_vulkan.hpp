#pragma once 

#include <rabbit/graphics/environment.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
	class environment_vulkan : public environment {
	public:
		environment_vulkan(VkDevice device,
			VkQueue graphics_queue,
			VkCommandPool command_pool,
			VmaAllocator allocator,
			VkDescriptorSetLayout descriptor_set_layout,
			const environment_desc& desc);

		~environment_vulkan();

		VkImage image() const;

		VkImageView image_view() const;

		VkSampler sampler() const;

		VkImage irradiance_image() const;

		VkImageView irradiance_image_view() const;

		VkSampler irradiance_sampler() const;

		VkImage prefilter_image() const;

		VkImageView prefilter_image_view() const;

		VkSampler prefilter_sampler() const;

		VkDescriptorSet descriptor_set() const;

	private:
		void _create_image(const environment_desc& desc);

		void _update_image(VkQueue graphics_queue, VkCommandPool command_pool, const environment_desc& desc);

		void _create_image_view(const environment_desc& desc);

		void _create_sampler(const environment_desc& desc);

		void _create_irradiance_image();

		void _create_prefilter_image();

		void _create_descriptor_set();

	private:
		VkDevice _device;
		VmaAllocator _allocator;

		VmaAllocation _allocation;
		VkImage _image;
		VkImageView _image_view;
		VkSampler _sampler;

		VmaAllocation _irradiance_allocation;
		VkImage _irradiance_image;
		VkImageView _irradiance_image_view;
		VkSampler _irradiance_sampler;

		VmaAllocation _prefilter_allocation;
		VkImage _prefilter_image;
		VkImageView _prefilter_image_view;
		VkSampler _prefilter_sampler;

		VkDescriptorSetLayout _descriptor_set_layout;
		VkDescriptorPool _descriptor_pool;
		VkDescriptorSet _descriptor_set;
	};
}
