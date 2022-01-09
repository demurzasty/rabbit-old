#include <rabbit/graphics/texture.hpp>
#include <rabbit/core/config.hpp>
#include <rabbit/graphics/graphics.hpp>
#include <rabbit/core/compression.hpp>
#include <rabbit/graphics/image.hpp>
#include <rabbit/graphics/s3tc.hpp>

using namespace rb;

inline std::uint32_t calculate_mipmap_levels(const vec2u& texture_size) {
	vec2u size{ texture_size };
	std::uint32_t mipmaps{ 0 };
	while (size.x > 4 && size.y > 4) {
		mipmaps++;
		size = size / 2u;
	}
	return mipmaps;
}

inline std::uint32_t calculate_bits_per_pixel(texture_format format) {
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
	
	const auto bits_per_pixel = calculate_bits_per_pixel(desc.format);
	const auto pixels = std::make_unique<std::uint8_t[]>(pixels_size);
	compression::uncompress(compressed_pixels.get(), compressed_size, pixels.get(), pixels_size);

	desc.data = pixels.get();
	return graphics::make_texture(desc);
}

void texture::import(ibstream& input, obstream& output, const json& metadata) {
	// 1. Load image from file to RGBA image
	auto image = image::load_from_stream(input);

	if (!image.is_power_of_two()) {
		const auto& size = image.size();
		image = image::resize(image, { next_power_of_two(size.x), next_power_of_two(size.y) });
	}
	
	RB_ASSERT(image, "Cannot load image.");
	RB_ASSERT(image.size().x % 4 == 0 && image.size().y % 4 == 0, "Incorrect texture size of image.");

	// Decide which format to use.
	texture_format format;
	if (metadata.contains("normal_map") && metadata["normal_map"]) {
		format = texture_format::rgba8; // TODO: Use bc4?
	} else if (metadata.contains("alpha") && metadata["alpha"]) {
		format = texture_format::bc3;
	} else {
		format = texture_format::bc1;
	}

	// Save base image size
	const auto base_size = image.size();

	// Calculate mipmap count based on image size
	const auto mipmap_count = std::min(calculate_mipmap_levels(image.size()), 6u);

	mobstream stream;
	for (auto i = 0u; i < mipmap_count; ++i) {
		if (format == texture_format::bc3) {
			// Compress mipmaps pixels to lossy, gpu friendly BC3
			const auto bc3_pixels = s3tc::bc3(image);
			RB_ASSERT(!bc3_pixels.empty(), "Cannot compress image.");
			stream.write(bc3_pixels.data(), bc3_pixels.size());
		} else if (format == texture_format::bc1) {
			// Compress mipmaps pixels to lossy, gpu friendly BC1
			const auto bc1_pixels = s3tc::bc1(image);
			RB_ASSERT(!bc1_pixels.empty(), "Cannot compress image.");
			stream.write(bc1_pixels.data(), bc1_pixels.size());
		} else {
			stream.write(image.pixels().data(), image.pixels().size_bytes());
		}

		image = image::resize(image, image.size() / 2u);
	}
	const auto pixels = stream.memory();

	// Compress to lossles, storage friendly zlib
	const auto compressed_pixels = compression::compress(pixels);

	RB_ASSERT(!compressed_pixels.empty(), "Cannot compress image.");

	output.write(texture::magic_number);
	output.write(base_size);
	output.write(format);
	output.write(texture_filter::linear);
	output.write(texture_wrap::repeat);
	output.write<std::uint32_t>(mipmap_count);
	output.write<std::uint32_t>(compressed_pixels.size());
	output.write<std::uint8_t>(compressed_pixels);
	output.write<std::uint32_t>(pixels.size_bytes());
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
	RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of texture should be greater than 0. Current size: {}, {}.", _size.x, _size.y);
	RB_ASSERT(_bits_per_pixel > 0, "Incorrect bits per pixel value: {}.", _bits_per_pixel);
}