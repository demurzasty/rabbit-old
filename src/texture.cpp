#include <rabbit/texture.hpp>
#include <rabbit/config.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/compression.hpp>

#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace rb;

inline std::size_t calculate_mipmap_levels(const vec2u& texture_size) {
	return (std::size_t)std::log2(std::max(texture_size.x, texture_size.y));
}

inline std::size_t calculate_bytes_per_pixel(texture_format format) {
	switch (format) {
		case texture_format::r8: return 1;
		case texture_format::rg8: return 2;
		case texture_format::rgba8: return 4;
	}

	return 0;
}

std::shared_ptr<texture> texture::load(bstream& stream) {
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
	
	const auto bytes_per_pixel = calculate_bytes_per_pixel(desc.format);
	const auto pixels = std::make_unique<std::uint8_t[]>(desc.size.x * desc.size.y * bytes_per_pixel);
	compression::uncompress(compressed_pixels.get(), compressed_size, pixels.get());

	desc.data = pixels.get();
	return graphics::make_texture(desc);
}

void texture::import(const std::string& input, const std::string& output, const json& metadata) {
	int width, height, components;
	std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels{
		stbi_load(input.c_str(), &width, &height, &components, STBI_rgb_alpha),
		&stbi_image_free
	};

	RB_ASSERT(pixels, "Cannot load image: {}", input);

	const auto compressed_bound = compression::compress_bound(width * height * 4);
	const auto compressed_pixels = std::make_unique<std::uint8_t[]>(compressed_bound);
	const auto compressed_size = compression::compress(pixels.get(), width * height * 4, compressed_pixels.get());

	RB_ASSERT(compressed_size > 0, "Cannot compress image");

	bstream stream{ output, bstream_mode::write };
	stream.write(width);
	stream.write(height);
	stream.write(texture_format::rgba8);
	stream.write(texture_filter::linear);
	stream.write(texture_wrap::repeat);
	stream.write(0u);
	stream.write<std::uint32_t>(compressed_size);
	stream.write(compressed_pixels.get(), compressed_size);
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

std::size_t texture::bytes_per_pixel() const {
	return _bytes_per_pixel;
}

texture::texture(const texture_desc& desc)
	: _size(desc.size)
	, _format(desc.format)
	, _filter(desc.filter)
	, _wrap(desc.wrap)
	, _mipmaps(desc.mipmaps > 0 ? desc.mipmaps : calculate_mipmap_levels(desc.size))
	, _bytes_per_pixel(calculate_bytes_per_pixel(desc.format)) {
}
