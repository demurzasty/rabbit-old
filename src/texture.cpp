#include <rabbit/texture.hpp>
#include <rabbit/config.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/compression.hpp>

#define STBI_MAX_DIMENSIONS 8192
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <rabbit/s3tc.hpp>

using namespace rb;

inline std::size_t calculate_mipmap_levels(const vec2u& texture_size) {
	return (std::size_t)std::log2(std::max(texture_size.x, texture_size.y));
}

inline std::size_t calculate_bits_per_pixel(texture_format format) {
	switch (format) {
		case texture_format::r8: return 8;
		case texture_format::rg8: return 16;
		case texture_format::rgba8: return 32;
		case texture_format::bc1: return 4;
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
	
	const auto bits_per_pixel = calculate_bits_per_pixel(desc.format);
	const auto pixels = std::make_unique<std::uint8_t[]>(desc.size.x * desc.size.y * bits_per_pixel / 8);
	compression::uncompress(compressed_pixels.get(), compressed_size, pixels.get());

	if (desc.format == texture_format::bc1) {
		desc.mipmaps = 1;
	}

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

	RB_ASSERT(width % 4 == 0 && height % 4 == 0, "Incorrect texture size");

	// 1. Compress pixels to lossy, gpu friendly BC1
	const auto bc1_size = width * height * calculate_bits_per_pixel(texture_format::bc1) / 8;
	const auto bc1_pixels = std::make_unique<std::uint8_t[]>(bc1_size);
	s3tc::bc1(pixels.get(), width * height * 4, width * 4, bc1_pixels.get());

	// 2. Compress to lossles, storage friendly zlib
	const auto compressed_bound = compression::compress_bound(bc1_size);
	const auto compressed_pixels = std::make_unique<std::uint8_t[]>(compressed_bound);
	const auto compressed_size = compression::compress(bc1_pixels.get(), bc1_size, compressed_pixels.get());

	RB_ASSERT(compressed_size > 0, "Cannot compress image");

	bstream stream{ output, bstream_mode::write };
	stream.write(texture::magic_number);
	stream.write(width);
	stream.write(height);
	stream.write(texture_format::bc1);
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

std::size_t texture::bits_per_pixel() const {
	return _bits_per_pixel;
}

texture::texture(const texture_desc& desc)
	: _size(desc.size)
	, _format(desc.format)
	, _filter(desc.filter)
	, _wrap(desc.wrap)
	, _mipmaps(desc.mipmaps > 0 ? desc.mipmaps : calculate_mipmap_levels(desc.size))
	, _bits_per_pixel(calculate_bits_per_pixel(desc.format)) {
}
