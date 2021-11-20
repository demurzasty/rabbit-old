#include <rabbit/editor/importers/texture_importer.hpp>
#include <rabbit/engine/core/config.hpp>
#include <rabbit/engine/core/compression.hpp>
#include <rabbit/engine/graphics/image.hpp>
#include <rabbit/engine/graphics/texture.hpp>
#include <rabbit/engine/graphics/s3tc.hpp>

using namespace rb;

void texture_importer::import(ibstream& input, obstream& output, const json& metadata) {
	// 1. Load image from file to RGBA image
	auto image = image::load_from_stream(input);
	
	RB_ASSERT(image, "Cannot load image.");
	RB_ASSERT(image.size().x % 4 == 0 && image.size().y % 4 == 0, "Incorrect texture size of image.");

	// Decide which format to use.
	texture_format format;
	if (metadata.contains("alpha") && metadata["alpha"]) {
		format = texture_format::bc3;
	} else {
		format = texture_format::bc1;
	}

	// Save base image size
	const auto base_size = image.size();

	// Calculate mipmap count based on image size
	const auto mipmap_count = texture_utils::calculate_mipmap_levels(image.size());

	mobstream stream;
	for (auto i = 0u; i < mipmap_count; ++i) {
		if (metadata.contains("alpha") && metadata["alpha"]) {
			// Compress mipmaps pixels to lossy, gpu friendly BC3
			const auto bc3_pixels = s3tc::bc3(image);
			RB_ASSERT(!bc3_pixels.empty(), "Cannot compress image.");
			stream.write(bc3_pixels.data(), bc3_pixels.size());
		} else {
			// Compress mipmaps pixels to lossy, gpu friendly BC1
			const auto bc1_pixels = s3tc::bc1(image);
			RB_ASSERT(!bc1_pixels.empty(), "Cannot compress image.");
			stream.write(bc1_pixels.data(), bc1_pixels.size());
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
