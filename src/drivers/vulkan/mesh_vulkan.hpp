#pragma once 

#include <rabbit/engine/graphics/mesh.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
	class mesh_vulkan : public mesh {
	public:
		mesh_vulkan(VkDevice device, VmaAllocator allocator, const mesh_desc& desc);

		~mesh_vulkan();

		VkBuffer vertex_buffer() const;

		VkBuffer index_buffer() const;

		VkBuffer clustered_index_buffer() const;

	private:
		VkDevice _device;
		VmaAllocator _allocator;

		VkBuffer _vertex_buffer;
		VmaAllocation _vertex_allocation;

		VkBuffer _index_buffer;
		VmaAllocation _index_allocation;

		VkBuffer _clustered_index_buffer;
		VmaAllocation _clustered_index_allocation;
	};
}
