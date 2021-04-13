#include <rabbit/graphics/texture_cube.hpp>

using namespace rb;

texture_cube::texture_cube(const texture_cube_desc& desc)
	: _size(desc.size)
	, _format(desc.format)
	, _filter(desc.filter)
	, _wrap(desc.wrap)
	, _is_render_target(desc.is_render_target)
	, _is_mutable(desc.is_mutable)
	, _mipmaps(desc.mipmaps) {
}

const vec2i& texture_cube::size() const {
	return _size;
}

vec2f texture_cube::texel() const {
	const auto& size = this->size();
	return { 1.0f / size.x, 1.0f / size.y };
}

texture_format texture_cube::format() const {
	return _format;
}

texture_filter texture_cube::filter() const {
	return _filter;
}

texture_wrap texture_cube::wrap() const {
	return _wrap;
}

bool texture_cube::is_render_target() const {
	return _is_render_target;
}

bool texture_cube::is_mutable() const {
	return _is_mutable;
}

int texture_cube::mipmaps() const {
	return _mipmaps;
}
