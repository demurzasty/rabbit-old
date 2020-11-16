#include <rabbit/texture.hpp>

using namespace rb;

texture::texture(const texture_desc& desc)
	: _size(desc.size)
	, _format(desc.format)
	, _filter(desc.filter)
	, _wrap(desc.wrap)
	, _is_render_target(desc.is_render_target)
	, _is_mutable(desc.is_mutable) {
}

const vec2i& texture::size() const {
	return _size;
}

vec2f texture::texel() const {
	const auto& size = this->size();
	return { 1.0f / size.x, 1.0f / size.y };
}

texture_format texture::format() const {
	return _format;
}

texture_filter texture::filter() const {
	return _filter;
}

texture_wrap texture::wrap() const {
	return _wrap;
}

bool texture::is_render_target() const {
	return _is_render_target;
}

bool texture::is_mutable() const {
	return _is_mutable;
}
