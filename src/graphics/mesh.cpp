#include <rabbit/graphics/mesh.hpp>
#include <rabbit/core/config.hpp>

using namespace rb;

span<const vec3f> mesh::positions() const noexcept {
    return _positions;
}

span<const vec3f> mesh::normals() const noexcept {
    return _normals;
}

span<const vec4f> mesh::tangents() const noexcept {
    return _tangents;
}

span<const vec2f> mesh::texcoords0() const noexcept {
    return _texcoords0;
}

span<const vec2f> mesh::texcoords1() const noexcept {
    return _texcoords1;
}

span<const color> mesh::colors() const noexcept {
    return _colors;
}

span<const std::uint32_t> mesh::indices() const noexcept {
    return _indices;
}

mesh::mesh(const mesh_desc& desc)
    : _positions(desc.positions.begin(), desc.positions.end())
    , _normals(desc.normals.begin(), desc.normals.end())
    , _tangents(desc.tangents.begin(), desc.tangents.end())
    , _texcoords0(desc.texcoords0.begin(), desc.texcoords0.end())
    , _texcoords1(desc.texcoords1.begin(), desc.texcoords1.end())
    , _colors(desc.colors.begin(), desc.colors.end())
    , _indices(desc.indices.begin(), desc.indices.end()) {
    RB_ASSERT(!_positions.empty(), "Positions buffer is mandatory.");
    RB_ASSERT(!_indices.empty(), "Index buffer is mandatory.");
}
