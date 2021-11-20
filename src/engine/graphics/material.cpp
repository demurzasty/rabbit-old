#include <rabbit/engine/graphics/material.hpp>
#include <rabbit/engine/graphics/graphics.hpp>
#include <rabbit/engine/core/assets.hpp>
#include <rabbit/engine/core/config.hpp>
#include <rabbit/engine/core/uuid.hpp>

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
    , _albedo_map(desc.albedo_map)
    , _normal_map(desc.normal_map)
    , _roughness_map(desc.roughness_map)
    , _metallic_map(desc.metallic_map)
    , _emissive_map(desc.emissive_map)
    , _ambient_map(desc.ambient_map)
    , _flags(calculate_material_flags(desc)) {
}
