#include <rabbit/graphics/material.hpp>
#include <rabbit/graphics/graphics.hpp>
#include <rabbit/core/assets.hpp>
#include <rabbit/core/config.hpp>
#include <rabbit/core/uuid.hpp>

#include <array>
#include <fstream>

using namespace rb;

inline std::uint32_t calculate_material_flags(const material_desc& desc) {
    std::uint32_t flags{ 0 };

    if (desc.translucent) {
        flags |= material_flags::translucent_bit;
    }

    if (desc.double_sided) {
        flags |= material_flags::double_sided_bit;
    }

    if (desc.wireframe) {
        flags |= material_flags::wireframe_bit;
    }

    if (desc.albedo_map) {
        flags |= material_flags::albedo_map_bit;
    }

    if (desc.normal_map) {
        flags |= material_flags::normal_map_bit;
    }

    if (desc.roughness_map) {
        flags |= material_flags::roughness_map_bit;
    }

    if (desc.metallic_map) {
        flags |= material_flags::metallic_map_bit;
    }

    if (desc.emissive_map) {
        flags |= material_flags::emissive_map_bit;
    }

    if (desc.ambient_map) {
        flags |= material_flags::ambient_map_bit;
    }

    return flags;
}

std::shared_ptr<material> material::load(ibstream& stream) {
    material_desc desc;
    stream.read(desc.base_color);
    stream.read(desc.roughness);
    stream.read(desc.metallic);
    stream.read(desc.occlusion_strength);
    stream.read(desc.translucent);
    stream.read(desc.double_sided);

    const auto read_uuid = [](ibstream& stream) -> uuid {
        uuid uuid;
        stream.read(uuid);
        return uuid;
    };

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.albedo_map = assets::load<texture>(uuid);
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.normal_map = assets::load<texture>(uuid);
    } 

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.roughness_map = assets::load<texture>(uuid);
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.metallic_map = assets::load<texture>(uuid);
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.emissive_map = assets::load<texture>(uuid);
    }

    if (const auto uuid = read_uuid(stream); uuid) {
        desc.ambient_map = assets::load<texture>(uuid);
    }

    return graphics::make_material(desc);
}

void material::import(ibstream& input, obstream& output, const json& metadata) {
    json json;
    input.read(json);

    vec4f base_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float roughness{ 0.8f };
    float metallic{ 0.0f };
    float occlusion_strength{ 1.0f };
    bool translucent{ false };
    bool double_sided{ false };

    if (json.contains("base_color")) {
        auto& color = json["base_color"];
        if (color.size() > 3) {
            base_color = { color[0], color[1], color[2], color[3] };
        } else {
            base_color = { color[0], color[1], color[2] };
        }
    }

    if (json.contains("roughness")) {
        roughness = json["roughness"];
    }

    if (json.contains("metallic")) {
        metallic = json["metallic"];
    }

    if (json.contains("occlusion_strength")) {
        occlusion_strength = json["occlusion_strength"];
    }

    if (json.contains("translucent")) {
        translucent = json["translucent"];
    }

    if (json.contains("double_sided")) {
        double_sided = json["double_sided"];
    }

    output.write(material::magic_number);
    output.write(base_color);
    output.write(roughness);
    output.write(metallic);
    output.write(occlusion_strength);
    output.write(translucent);
    output.write(double_sided);
    
    if (json.contains("albedo_map")) {
        output.write(uuid::from_string(json["albedo_map"]).value());
    } else {
        output.write(uuid{}.data());
    }

    if (json.contains("normal_map")) {
        output.write(uuid::from_string(json["normal_map"]).value());
    } else {
        output.write(uuid{}.data());
    }

    if (json.contains("roughness_map")) {
        output.write(uuid::from_string(json["roughness_map"]).value());
    } else {
        output.write(uuid{}.data());
    }

    if (json.contains("metallic_map")) {
        output.write(uuid::from_string(json["metallic_map"]).value());
    } else {
        output.write(uuid{}.data());
    }

    if (json.contains("emissive_map")) {
        output.write(uuid::from_string(json["emissive_map"]).value());
    } else {
        output.write(uuid{}.data());
    }

    if (json.contains("ambient_map")) {
        output.write(uuid::from_string(json["ambient_map"]).value());
    } else {
        output.write(uuid{}.data());
    }
}

const vec4f& material::base_color() const {
    return _base_color;
}

float material::roughness() const {
    return _roughness;
}

float material::metallic() const {
    return _metallic;
}

bool material::translucent() const {
    return _translucent;
}

bool material::double_sided() const {
    return _double_sided;
}

bool material::wireframe() const {
    return _wireframe;
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

const std::shared_ptr<texture>& material::ambient_map() const {
    return _ambient_map;
}

const std::uint32_t material::flags() const {
    return _flags;
}

material::material(const material_desc& desc)
    : _base_color(desc.base_color)
    , _roughness(desc.roughness)
    , _metallic(desc.metallic)
    , _translucent(desc.translucent)
    , _double_sided(desc.double_sided)
    , _wireframe(desc.wireframe)
    , _albedo_map(desc.albedo_map)
    , _normal_map(desc.normal_map)
    , _roughness_map(desc.roughness_map)
    , _metallic_map(desc.metallic_map)
    , _emissive_map(desc.emissive_map)
    , _ambient_map(desc.ambient_map)
    , _flags(calculate_material_flags(desc)) {
}
