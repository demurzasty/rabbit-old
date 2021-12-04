#include <rabbit/graphics/environment.hpp>
#include <rabbit/core/config.hpp>
#include <rabbit/graphics/graphics.hpp>
#include <rabbit/core/compression.hpp>
#include <rabbit/graphics/image.hpp>

#include <array>
#include <fstream>

using namespace rb;

static std::array<const char*, 6> faces = {
    "right",
    "left",
    "top",
    "bottom",
    "front",
    "back"
};

std::shared_ptr<environment> environment::load(ibstream& stream) {
    environment_desc desc;
    stream.read(desc.size);

    std::uint32_t compressed_size;
    stream.read(compressed_size);

    const auto compressed_pixels = std::make_unique<std::uint8_t[]>(compressed_size);
    stream.read(compressed_pixels.get(), compressed_size);

    const auto uncompressed_pixels = std::make_unique<std::uint8_t[]>(desc.size.x * desc.size.y * 4 * 6);
    compression::uncompress(compressed_pixels.get(), compressed_size, uncompressed_pixels.get(), desc.size.x * desc.size.y * 4 * 6);

    desc.data = uncompressed_pixels.get();
    return graphics::make_environment(desc);
}

void environment::import(ibstream& input, obstream& output, const json& metadata) {
    json json;
    input.read(json);

    // Get individual faces texture filenames.
    std::array<std::string, 6> filenames;
    for (std::size_t index{ 0 }; index < 6; ++index) {
        filenames[index] = json[faces[index]];
    }

    vec2u size;

    std::map<std::size_t, image> images;
    for (std::size_t index{ 0 }; index < 6; ++index) {
        images[index] = image::load_from_file(filenames[index]);
        size = images[index].size();
    }

    std::vector<color> buffer(size.x * size.y * 6);
    for (auto& [index, images] : images) {
        std::memcpy(buffer.data() + index * size.x * size.y, images.pixels().data(), size.x * size.y * sizeof(color));
    }

    const auto compressed_pixels = compression::compress<color>(buffer);
    
    output.write(environment::magic_number);
    output.write(size);
    output.write<std::uint32_t>(compressed_pixels.size());
    output.write<std::uint8_t>(compressed_pixels);
}

const vec2u& environment::size() const {
	return _size;
}

environment::environment(const environment_desc& desc)
	: _size(desc.size) {
    RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of environment map should be greater than 0. Current size: {}, {}.", _size.x, _size.y);
}
