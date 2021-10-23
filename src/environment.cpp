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

std::shared_ptr<environment> environment::load(const std::string& filename, json& metadata) {
    // Try to open file.
    std::fstream stream{ filename, std::ios::in };
    RB_ASSERT(stream.is_open(), "Cannot open file");

    // Load json from file.
    json json;
    stream >> json;

    // We do not need open stream anymore.
    stream.close();

    // Get individual faces texture filenames.
    std::array<std::string, 6> filenames;
    for (std::size_t index{ 0 }; index < 6; ++index) {
        filenames[index] = json[faces[index]];
    }

    environment_desc desc;

    std::map<std::size_t, stbi_uc*> data;
    for (std::size_t index{ 0 }; index < 6; ++index) {
        int width, height, components;
        data[index] = stbi_load(filenames[index].c_str(), &width, &height, &components, STBI_rgb_alpha);
        desc.size = { static_cast<unsigned int>(width), static_cast<unsigned int>(height) };
    }

    auto buffer = std::make_unique<stbi_uc[]>(desc.size.x * desc.size.y * 4 * 6);
    for (auto& [index, pixels] : data) {
        std::memcpy(buffer.get() + index * (desc.size.x * desc.size.y * 4), pixels, desc.size.x * desc.size.y * 4);
        stbi_image_free(pixels);
    }

    desc.data = buffer.get();
    return graphics::make_environment(desc);
}

const vec2u& environment::size() const {
	return _size;
}

environment::environment(const environment_desc& desc)
	: _size(desc.size) {
}
