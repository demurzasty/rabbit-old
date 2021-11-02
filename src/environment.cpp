#include <rabbit/environment.hpp>
#include <rabbit/config.hpp>
#include <rabbit/graphics.hpp>

#include <array>
#include <fstream>

#include <stb_image.h>

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

    const auto data = std::make_unique<std::uint8_t[]>(desc.size.x * desc.size.y * 4 * 6);
    stream.read(data.get(), desc.size.x * desc.size.y * 4 * 6);
    desc.data = data.get();
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

    std::map<std::size_t, stbi_uc*> data;
    for (std::size_t index{ 0 }; index < 6; ++index) {
        int width, height, components;
        data[index] = stbi_load(filenames[index].c_str(), &width, &height, &components, STBI_rgb_alpha);
        size = { static_cast<unsigned int>(width), static_cast<unsigned int>(height) };
    }

    auto buffer = std::make_unique<stbi_uc[]>(size.x * size.y * 4 * 6);
    for (auto& [index, pixels] : data) {
        std::memcpy(buffer.get() + index * (size.x * size.y * 4), pixels, size.x * size.y * 4);
        stbi_image_free(pixels);
    }

    bstream stream{ output, bstream_mode::write };
    stream.write(environment::magic_number);
    stream.write(size);
    stream.write(buffer.get(), size.x * size.y * 4 * 6);
}

const vec2u& environment::size() const {
	return _size;
}

environment::environment(const environment_desc& desc)
	: _size(desc.size) {
}
