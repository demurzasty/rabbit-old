#include <rabbit/engine/graphics/texture.hpp>
#include <rabbit/engine/graphics/graphics.hpp>
#include <rabbit/engine/core/config.hpp>
#include <rabbit/engine/core/compression.hpp>

using namespace rb;

std::size_t texture_utils::calculate_mipmap_levels(const vec2u& texture_size) {
	vec2u size{ texture_size };
	std::size_t mipmaps{ 0 };
	while (size.x > 4 && size.y > 4) {
		mipmaps++;
		size = size / 2u;
	}
	return mipmaps;
}

std::size_t texture_utils::calculate_bits_per_pixel(texture_format format) {
	switch (format) {
		case texture_format::r8: return 8;
		case texture_format::rg8: return 16;
		case texture_format::rgba8: return 32;
		case texture_format::bc1: return 4;
		case texture_format::bc3: return 8;
	}

	return 0;
}

std::shared_ptr<texture> texture::load(ibstream& stream) {
	texture_desc desc;
	stream.read(desc.size.x);
	stream.read(desc.size.y);
	stream.read(desc.format);
	stream.read(desc.filter);
	stream.read(desc.wrap);
	stream.read(desc.mipmaps);

	std::uint32_t compressed_size;
	stream.read(compressed_size);

	const auto compressed_pixels = std::make_unique<std::uint8_t[]>(compressed_size);
	stream.read(compressed_pixels.get(), compressed_size);

	std::uint32_t pixels_size;
	stream.read(pixels_size);
	
	const auto pixels = std::make_unique<std::uint8_t[]>(pixels_size);
	compression::uncompress(compressed_pixels.get(), compressed_size, pixels.get(), pixels_size);

	desc.data = pixels.get();
	return graphics::make_texture(desc);
}

std::shared_ptr<texture> texture::make_one_color(const color& color, const vec2u& size) {
	const auto pixels = std::make_unique<rb::color[]>(size.x * size.y);
	for (auto i = 0u; i < size.x * size.y; ++i) {
		pixels[i] = color;
	}

	texture_desc desc;
	desc.filter = texture_filter::linear;
	desc.wrap = texture_wrap::repeat;
	desc.mipmaps = 1;
	desc.size = size;
	desc.format = texture_format::rgba8;
	desc.data = pixels.get();
	return graphics::make_texture(desc);
}

const vec2u& texture::size() const {
	return _size;
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

std::uint32_t texture::mipmaps() const {
	return _mipmaps;
}

std::size_t texture::bits_per_pixel() const {
	return _bits_per_pixel;
}

texture::texture(const texture_desc& desc)
	: _size(desc.size)
	, _format(desc.format)
	, _filter(desc.filter)
	, _wrap(desc.wrap)
	, _mipmaps(desc.mipmaps > 0 ? desc.mipmaps : texture_utils::calculate_mipmap_levels(desc.size))
	, _bits_per_pixel(texture_utils::calculate_bits_per_pixel(desc.format)) {
	RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of texture should be greater than 0. Current size: {}, {}.", _size.x, _size.y);
	RB_ASSERT(_bits_per_pixel > 0, "Incorrect bits per pixel value: {}.", _bits_per_pixel);
}
