#include <rabbit/graphics/texture.hpp>
#include <rabbit/core/config.hpp>

using namespace rb;

std::uint32_t texture_utils::mipmap_levels(const vec2u& texture_size) {
	vec2u size{ texture_size };
	std::uint32_t mipmaps{ 0 };
	while (size.x > 4 && size.y > 4) {
		mipmaps++;
		size = size / 2u;
	}
	return mipmaps;
}

std::uint32_t texture_utils::bits_per_pixel(texture_format format) {
	switch (format) {
		case texture_format::r8: return 8;
		case texture_format::rg8: return 16;
		case texture_format::rgba8: return 32;
		case texture_format::bc1: return 4;
		case texture_format::bc3: return 8;
	}

	return 0;
}

const vec2u& texture::size() const noexcept {
	return _size;
}

texture_format texture::format() const noexcept {
	return _format;
}

texture_filter texture::filter() const noexcept {
	return _filter;
}

texture_wrap texture::wrap() const noexcept {
	return _wrap;
}

std::uint32_t texture::mipmaps() const noexcept {
	return _mipmaps;
}

std::uint32_t texture::bits_per_pixel() const noexcept {
	return _bits_per_pixel;
}

texture::texture(const texture_desc& desc)
	: _size(desc.size)
	, _format(desc.format)
	, _filter(desc.filter)
	, _wrap(desc.wrap)
	, _mipmaps(desc.mipmaps > 0 ? desc.mipmaps : texture_utils::mipmap_levels(desc.size))
	, _bits_per_pixel(texture_utils::bits_per_pixel(desc.format)) {
	RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of texture should be greater than 0. Current size: {}, {}.", _size.x, _size.y);
	RB_ASSERT(_bits_per_pixel > 0, "Incorrect bits per pixel value: {}.", _bits_per_pixel);
}
