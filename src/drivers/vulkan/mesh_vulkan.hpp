#pragma once 

#include <rabbit/graphics/mesh.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
	class mesh_vulkan : public mesh {
	public:
		mesh_vulkan(std::uint32_t vertex_offset, std::uint32_t index_offset, const mesh_desc& desc);

		std::uint32_t vertex_offset() const;

		std::uint32_t index_offset() const;

	private:
		std::uint32_t _vertex_offset;
		std::uint32_t _index_offset;
	};
}
