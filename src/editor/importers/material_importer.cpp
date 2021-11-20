#include <rabbit/editor/importers/material_importer.hpp>
#include <rabbit/engine/graphics/material.hpp>

using namespace rb;

void material_importer::import(ibstream& input, obstream& output, const json& metadata) {
    json json;
    input.read(json);

    vec4f base_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float roughness{ 0.8f };
    float metallic{ 0.0f };
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
