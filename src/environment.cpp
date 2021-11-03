#include <rabbit/environment.hpp>
#include <rabbit/config.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/compression.hpp>
#include <rabbit/image.hpp>

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

std::shared_ptr<environment> environment::load(bstream& stream) {
    environment_desc desc;
    stream.read(desc.size);

    std::uint32_t compressed_size;
    stream.read(compressed_size);

    const auto compressed_pixels = std::make_unique<std::uint8_t[]>(compressed_size);
    stream.read(compressed_pixels.get(), compressed_size);

    const auto uncompressed_pixels = std::make_unique<std::uint8_t[]>(desc.size.x * desc.size.y * 4 * 6);
    compression::uncompress(compressed_pixels.get(), compressed_size, uncompressed_pixels.get());

    desc.data = uncompressed_pixels.get();
    return graphics::make_environment(desc);
}

void environment::import(const std::string& input, const std::string& output, const json& metadata) {
    // Try to open file.
    std::fstream istream{ input, std::ios::in };
    RB_ASSERT(istream.is_open(), "Cannot open file");

    // Load json from file.
    json json;
    istream >> json;

    // We do not need open stream anymore.
    istream.close();

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

    bstream stream{ output, bstream_mode::write };
    stream.write(environment::magic_number);
    stream.write(size);
    stream.write<std::uint32_t>(compressed_pixels.size());
    stream.write<std::uint8_t>(compressed_pixels);
}

const vec2u& environment::size() const {
	return _size;
}

environment::environment(const environment_desc& desc)
	: _size(desc.size) {
    RB_ASSERT(_size.x > 0 && _size.y > 0, "Size of environment map should be greater than 0. Current size: {}, {}.", _size.x, _size.y);
}
