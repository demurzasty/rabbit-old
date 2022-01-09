#include "mesh_vulkan.hpp"
#include "utils_vulkan.hpp"

using namespace rb;

mesh_vulkan::mesh_vulkan(std::uint32_t vertex_offset, std::uint32_t index_offset, const mesh_desc& desc)
	: mesh(desc)
	, _vertex_offset(vertex_offset)
	, _index_offset(index_offset) {
}

std::uint32_t mesh_vulkan::vertex_offset() const {
    return _vertex_offset;
}

std::uint32_t mesh_vulkan::index_offset() const {
    return _index_offset;
}
