#include <rabbit/material.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/assets.hpp>
#include <rabbit/config.hpp>
#include <rabbit/uuid.hpp>

#include <array>
#include <fstream>

using namespace rb;

std::shared_ptr<material> material::load(bstream& stream) {
    material_desc desc;
    stream.read(desc.base_color);
    stream.read(desc.roughness);
    stream.read(desc.metallic);

    const auto read_uuid = [](bstream& stream) -> uuid {
        uuid uuid;
        stream.read(uuid);
        return uuid;
    };

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.albedo_map = assets::load<texture>(uuid);
    } else {
        desc.albedo_map = texture::make_one_color(color::white(), { 2, 2 });
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.normal_map = assets::load<texture>(uuid);
    } else {
        desc.normal_map = texture::make_one_color({ 127, 127, 255, 255 }, { 2, 2 });
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.roughness_map = assets::load<texture>(uuid);
    } else {
        desc.roughness_map = texture::make_one_color(color::white(), { 2, 2 });
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.metallic_map = assets::load<texture>(uuid);
    } else {
        desc.metallic_map = texture::make_one_color(color::white(), { 2, 2 });
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.emissive_map = assets::load<texture>(uuid);
    } else {
        desc.emissive_map = texture::make_one_color(color::black(), { 2, 2 });
    }

    return graphics::make_material(desc);
}

void material::import(const std::string& input, const std::string& output, const json& metadata) {
    std::ifstream istream{ input, std::ios::in };
    RB_ASSERT(istream.is_open(), "Cannot open file");

    json json;
    istream >> json;
    istream.close();

    vec3f base_color{ 1.0f, 1.0f, 1.0f };
    float roughness{ 0.8f };
    float metallic{ 0.0f };

    if (json.contains("base_color")) {
        auto& color = json["base_color"];
        base_color = { color[0], color[1], color[2] };
    }

    if (json.contains("roughness")) {
        roughness = json["roughness"];
    }

    if (json.contains("metallic")) {
        metallic = json["metallic"];
    }

    bstream stream{ output, bstream_mode::write };
    stream.write(base_color);
    stream.write(roughness);
    stream.write(metallic);
    
    if (json.contains("albedo_map")) {
        stream.write(uuid::from_string(json["albedo_map"]).value().data());
    } else {
        stream.write(uuid{}.data());
    }

    if (json.contains("normal_map")) {
        stream.write(uuid::from_string(json["normal_map"]).value().data());
    } else {
        stream.write(uuid{}.data());
    }

    if (json.contains("roughness_map")) {
        stream.write(uuid::from_string(json["roughness_map"]).value().data());
    } else {
        stream.write(uuid{}.data());
    }

    if (json.contains("metallic_map")) {
        stream.write(uuid::from_string(json["metallic_map"]).value().data());
    } else {
        stream.write(uuid{}.data());
    }

    if (json.contains("emissive_map")) {
        stream.write(uuid::from_string(json["emissive_map"]).value().data());
    } else {
        stream.write(uuid{}.data());
    }
}

const vec3f& material::base_color() const {
    return _base_color;
}

float material::roughness() const {
    return _roughness;
}

float material::metallic() const {
    return _metallic;
}

const std::shared_ptr<texture>& material::albedo_map() const {
    return _albedo_map;
}

const std::shared_ptr<texture>& material::normal_map() const {
    return _normal_map;
}

const std::shared_ptr<texture>& material::roughness_map() const {
    return _roughness_map;
}

const std::shared_ptr<texture>& material::metallic_map() const {
    return _metallic_map;
}

const std::shared_ptr<texture>& material::emissive_map() const {
    return _emissive_map;
}

material::material(const material_desc& desc)
    : _base_color(desc.base_color)
    , _roughness(desc.roughness)
    , _metallic(desc.metallic)
    , _albedo_map(desc.albedo_map)
    , _normal_map(desc.normal_map)
    , _roughness_map(desc.roughness_map)
    , _metallic_map(desc.metallic_map)
    , _emissive_map(desc.emissive_map) {
}
