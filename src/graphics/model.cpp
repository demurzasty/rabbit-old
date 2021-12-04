#include <rabbit/graphics/model.hpp>
#include <rabbit/graphics/mesh.hpp>
#include <rabbit/graphics/material.hpp>
#include <rabbit/core/format.hpp>
#include <rabbit/core/prefab.hpp>
#include <rabbit/math/math.hpp>
#include <rabbit/math/quat.hpp>
#include <rabbit/editor/editor.hpp>

#include <vector>
#include <filesystem>
#include <fstream>

using namespace rb;

static json parse_node(const json& gltf_meshes,
    const std::vector<std::vector<std::string>>& meshes,
    const json& gltf_materials,
    const std::vector<std::string>& materials,
    const json& gltf_nodes,
    const json& gltf_node) {
    json jentity;

    if (gltf_node.contains("name")) {
        jentity["identity"] = {
            { "name", gltf_node["name"] }
        };
    }

    vec3f rotation{ 0.0f, 0.0f, 0.0f };
    if (gltf_node.contains("rotation")) {
        const auto& gltf_rotation = gltf_node["rotation"];
        const float quat[4]{
            gltf_rotation[0],
            gltf_rotation[1],
            gltf_rotation[2],
            gltf_rotation[3]
        };

        const auto sinr_cosp = 2.0f * (quat[3] * quat[0] + quat[1] * quat[2]);
        const auto cosr_cosp = 1.0f - 2.0f * (quat[0] * quat[0] + quat[1] * quat[1]);
        rotation.x = std::atan2(sinr_cosp, cosr_cosp);

        const auto sinp = 2.0f * (quat[3] * quat[1] - quat[2] * quat[0]);
        if (std::abs(sinp) >= 1.0f) {
            rotation.y = std::copysign(pi<float>(), sinp);
        } else {
            rotation.y = std::asin(sinp);
        }

        const auto siny_cosp = 2.0f * (quat[3] * quat[2] + quat[0] * quat[1]);
        const auto cosy_cosp = 1.0f - 2.0f * (quat[1] * quat[1] + quat[2] * quat[2]);
        rotation.z = std::atan2(siny_cosp, cosy_cosp);
    }

    vec3f scaling{ 1.0f, 1.0f, 1.0f };
    if (gltf_node.contains("scale")) {
        const auto& gltf_scale = gltf_node["scale"];
        scaling = { gltf_scale[0], gltf_scale[1], gltf_scale[2] };
    }

    vec3f position{ 0.0f, 0.0f, 0.0f };
    if (gltf_node.contains("matrix")) {
        const auto& gltf_matrix = gltf_node["matrix"];
        
        mat4f matrix;
        for (auto i = 0u; i < 16u; ++i) {
            matrix[i] = gltf_matrix[i];
        }

        position = { matrix[12], matrix[13], matrix[14] };

        scaling.x = length(vec3f{ matrix[0], matrix[1], matrix[2] });
        scaling.y = length(vec3f{ matrix[4], matrix[5], matrix[6] });
        scaling.z = length(vec3f{ matrix[8], matrix[9], matrix[10] });

        if (determinant(matrix) <= 0.0f) {
            scaling.y *= -1.0f;
        }

        if (scaling.x == 0 || scaling.y == 0 || scaling.z == 0) {
            rotation = { 0.0f, 0.0f, 0.0f };
        } else {
            const auto sx = 1.0f / scaling.x;
            const auto sy = 1.0f / scaling.y;
            const auto sz = 1.0f / scaling.z;

            mat4f temp{ {
                matrix[0] * sx, matrix[1] * sx, matrix[2] * sx, 0,
                matrix[4] * sy, matrix[5] * sy, matrix[6] * sy, 0,
                matrix[8] * sz, matrix[9] * sz, matrix[10] * sz, 0,
                0, 0, 0, 1,
            } };

            rotation = euler_angles(mat4f::to_quat(temp));
        }
    } else if (gltf_node.contains("translation")) {
        const auto& gltf_translation = gltf_node["translation"];
        position = { gltf_translation[0], gltf_translation[1], gltf_translation[2] };
    }

    jentity["transform"] = {
        { "position", { position.x, position.y, position.z } },
        { "rotation", { rotation.x, rotation.y, rotation.z } },
        { "scaling", { scaling.x, scaling.y, scaling.z } },
    };

    if (gltf_node.contains("mesh")) {
        const std::size_t mesh_index = gltf_node["mesh"];
        const auto& gltf_mesh = gltf_meshes[mesh_index];
        const auto& gltf_primitives = gltf_mesh["primitives"];

        if (gltf_primitives.size() == 1) {
            const auto& gltf_primitive = gltf_primitives[0];
            const std::size_t material_index = gltf_primitive["material"];

            jentity["geometry"] = {
                { "mesh", meshes[mesh_index][0] },
                { "material", materials[material_index] }
            };
        } else {
            json jentity_primitive;
            jentity_primitive["transform"] = {
                { "position", { 0.0f, 0.0f, 0.0f } },
                { "rotation", { 0.0f, 0.0f, 0.0f } },
                { "scaling", { 1.0f, 1.0f, 1.0f } },
            };

            std::size_t primitive_index{ 0 };
            for (const auto& gltf_primitive : gltf_primitives) {
                const size_t material_index = gltf_primitive["material"];

                jentity_primitive["geometry"] = {
                    { "mesh", meshes[mesh_index][primitive_index] },
                    { "material", materials[material_index] }
                };

                jentity["children"].push_back(jentity_primitive);
                primitive_index++;
            }
        }
    }

    if (gltf_node.contains("children")) {
        auto& children = jentity["children"];

        for (const auto& gltf_child : gltf_node["children"]) {
            const std::size_t child_index = gltf_child;
            const auto jchild = parse_node(gltf_meshes, meshes, gltf_materials, materials, gltf_nodes, gltf_nodes[child_index]);
            children.push_back(jchild);
        }
    }

    return jentity;
}

void model::import(ibstream& input, obstream& output, const json& metadata) {
    // 0. Parse glTF as json.
    const auto gltf = input.read<json>();

    // 1. Prepare main informations.
    const auto& gltf_buffers = gltf["buffers"];
    const auto& gltf_buffer_views = gltf["bufferViews"];
    const auto& gltf_accessors = gltf["accessors"];
    const auto& gltf_images = gltf["images"];
    const auto& gltf_textures = gltf["textures"];
    const auto& gltf_materials = gltf["materials"];
    const auto& gltf_meshes = gltf["meshes"];
    const auto& gltf_nodes = gltf["nodes"];
    const auto& gltf_scenes = gltf["scenes"];

    const std::filesystem::path path{ static_cast<std::string>(metadata["_path"]) };
    const auto directory_path = path.parent_path();
    const auto data_path = directory_path / (path.filename().string() + ".data");

    std::filesystem::create_directory(data_path);

    // 2. Read buffers.
    std::vector<std::vector<std::uint8_t>> buffers;
    for (const auto& gltf_buffer : gltf_buffers) {
        const std::size_t buffer_size = gltf_buffer["byteLength"];
        const std::string buffer_uri = gltf_buffer["uri"];

        std::vector<std::uint8_t> buffer(buffer_size);

        fibstream stream{ (directory_path / buffer_uri).string() };
        stream.read(buffer.data(), buffer_size);
        buffers.push_back(buffer);
    }

    // 2. Import materials.
    std::vector<std::string> materials;
    std::size_t material_index{ 0 };
    for (const auto& gltf_material : gltf["materials"]) {
        json jmaterial;

        // Read PBR data.
        if (gltf_material.contains("pbrMetallicRoughness")) {
            const auto& gltf_material_pbr = gltf_material["pbrMetallicRoughness"];
            if (gltf_material_pbr.contains("baseColorFactor")) {
                jmaterial["base_color"] = gltf_material_pbr["baseColorFactor"];
            } else {
                jmaterial["base_color"] = { 1.0f, 1.0f, 1.0f, 1.0f };
            }

            if (gltf_material_pbr.contains("baseColorTexture")) {
                const std::size_t texture_index = gltf_material_pbr["baseColorTexture"]["index"];
                const auto& texture = gltf_textures[texture_index];

                const std::size_t image_index = texture["source"];
                const auto& image = gltf_images[image_index];

                // TODO: Self-contained images support.
                if (image.contains("uri")) {
                    const std::string image_filename = image["uri"];
                    const auto image_path = directory_path / image_filename;
                    const auto image_metadata_path = image_path.string() + ".meta";
                    if (std::filesystem::exists(image_metadata_path)) {
                        json image_metadata;
                        std::ifstream{ image_metadata_path } >> image_metadata;
                        jmaterial["albedo_map"] = image_metadata["uuid"];

                        if (gltf_material.contains("alphaMode") &&
                            gltf_material["alphaMode"] == "BLEND") {
                            image_metadata["alpha"] = true;
                            std::ofstream{ image_metadata_path } << image_metadata;
                        }
                    }
                }
            }

            if (gltf_material_pbr.contains("metallicFactor")) {
                jmaterial["metallic"] = gltf_material_pbr["metallicFactor"];
            } else {
                jmaterial["metallic"] = 1.0f;
            }

            if (gltf_material_pbr.contains("roughnessFactor")) {
                jmaterial["roughness"] = gltf_material_pbr["roughnessFactor"];
            } else {
                jmaterial["roughness"] = 1.0f;
            }

            if (gltf_material_pbr.contains("metallicRoughnessTexture")) {
                const std::size_t texture_index = gltf_material_pbr["metallicRoughnessTexture"]["index"];
                const auto& texture = gltf_textures[texture_index];

                const std::size_t image_index = texture["source"];
                const auto& image = gltf_images[image_index];

                // TODO: Self-contained images support.
                if (image.contains("uri")) {
                    const std::string image_filename = image["uri"];
                    const auto image_path = directory_path / image_filename;
                    const auto image_metadata_path = image_path.string() + ".meta";
                    if (std::filesystem::exists(image_metadata_path)) {
                        json image_metadata;
                        std::ifstream{ image_metadata_path } >> image_metadata;
                        jmaterial["metallic_map"] = image_metadata["uuid"];
                        jmaterial["roughness_map"] = image_metadata["uuid"];
                    }
                }
            }
        }

        // Read normal texture.
        if (gltf_material.contains("normalTexture")) {
            const std::size_t texture_index = gltf_material["normalTexture"]["index"];
            const auto& texture = gltf_textures[texture_index];

            const std::size_t image_index = texture["source"];
            const auto& image = gltf_images[image_index];

            // TODO: Self-contained images support.
            if (image.contains("uri")) {
                const std::string image_filename = image["uri"];
                const auto image_path = directory_path / image_filename;
                const auto image_metadata_path = image_path.string() + ".meta";
                if (std::filesystem::exists(image_metadata_path)) {
                    json image_metadata;
                    std::ifstream{ image_metadata_path } >> image_metadata;
                    jmaterial["normal_map"] = image_metadata["uuid"];

                    // Flag texture as normal map.
                    image_metadata["normal_map"] = true;
                    std::ofstream{ image_metadata_path } << image_metadata;
                }
            }
        }

        // Read occlusion texture.
        if (gltf_material.contains("occlusionTexture")) {
            const auto& gltf_occlusion_texture = gltf_material["occlusionTexture"];
            const std::size_t texture_index = gltf_occlusion_texture["index"];
            const auto& texture = gltf_textures[texture_index];

            const std::size_t image_index = texture["source"];
            const auto& image = gltf_images[image_index];

            // TODO: Self-contained images support.
            if (image.contains("uri")) {
                const std::string image_filename = image["uri"];
                const auto image_path = directory_path / image_filename;
                const auto image_metadata_path = image_path.string() + ".meta";
                if (std::filesystem::exists(image_metadata_path)) {
                    json image_metadata;
                    std::ifstream{ image_metadata_path } >> image_metadata;
                    jmaterial["ambient_map"] = image_metadata["uuid"];
                }
            }

            if (gltf_occlusion_texture.contains("strength")) {
                jmaterial["occlusion_strength"] = gltf_occlusion_texture["strength"];
            }
        }

        // Read emissive texture.
        if (gltf_material.contains("emissiveTexture")) {
            const std::size_t texture_index = gltf_material["emissiveTexture"]["index"];
            const auto& texture = gltf_textures[texture_index];

            const std::size_t image_index = texture["source"];
            const auto& image = gltf_images[image_index];

            // TODO: Self-contained images support.
            if (image.contains("uri")) {
                const std::string image_filename = image["uri"];
                const auto image_path = directory_path / image_filename;
                const auto image_metadata_path = image_path.string() + ".meta";
                if (std::filesystem::exists(image_metadata_path)) {
                    json image_metadata;
                    std::ifstream{ image_metadata_path } >> image_metadata;
                    jmaterial["emissive_map"] = image_metadata["uuid"];
                }
            }
        }

        if (gltf_material.contains("alphaMode") && gltf_material["alphaMode"] == "BLEND") {
            jmaterial["translucent"] = true;
        }

        if (gltf_material.contains("doubleSided") && gltf_material["doubleSided"]) {
            jmaterial["double_sided"] = true;
        }

        // TODO: Emissive factor support.
        // TODO: Alpha cutoff support.

        const std::string material_name = format("material{}", material_index);
        const auto material_filename = material_name + ".mat";
        const auto material_metadata_filename = material_filename + ".meta";

        std::ofstream material_stream{ data_path / material_filename };
        if (material_stream.is_open()) {
            material_stream << std::setw(4) << jmaterial;
        }
        material_stream.close();

        json jmaterial_metadata;
        if (std::filesystem::exists(data_path / material_metadata_filename)) {
            std::ifstream material_metadata_stream{ data_path / material_metadata_filename };
            material_metadata_stream >> jmaterial_metadata;
        } else {
            std::ofstream material_metadata_stream{ data_path / material_metadata_filename };
            jmaterial_metadata["uuid"] = uuid::generate().to_string();
            material_metadata_stream << jmaterial_metadata;
        }

        editor::import((data_path / material_filename).string());

        materials.push_back(jmaterial_metadata["uuid"]);
        material_index++;
    }

    // 3. Import meshes.
    std::vector<std::vector<std::string>> meshes(gltf_meshes.size());
    std::size_t mesh_index{ 0 };
    for (const auto& gltf_mesh : gltf_meshes) {
        std::size_t primitive_index{ 0 };
        for (const auto& gltf_primitive : gltf_mesh["primitives"]) {
            const std::size_t indices_accessor_index = gltf_primitive["indices"];
            const auto& gltf_index_accessor = gltf_accessors[indices_accessor_index];

            const std::size_t index_count = gltf_index_accessor["count"];
            const std::size_t byte_offset = gltf_index_accessor.contains("byteOffset") ?
                (unsigned int)gltf_index_accessor["byteOffset"] : 0u;

            std::vector<std::uint32_t> indices(index_count);
            
            const std::size_t index_buffer_view_index = gltf_index_accessor["bufferView"];
            const auto& gltf_index_buffer_view = gltf_buffer_views[index_buffer_view_index];

            const std::size_t index_buffer_index = gltf_index_buffer_view["buffer"];
            const std::size_t index_buffer_offset = gltf_index_buffer_view.contains("byteOffset") ?
                (unsigned int)gltf_index_buffer_view["byteOffset"] : 0u;
            //const auto& gltf_index_buffer = gltf_buffers[index_buffer_index];
            const auto& index_buffer = buffers[index_buffer_index];

            const std::size_t index_type = gltf_index_accessor["componentType"];
            const auto index_stride = index_type == 5123 ? 2 : 4;

            const std::uint8_t* data = index_buffer.data() + (index_buffer_offset + byte_offset);
            for (std::size_t index{ 0 }; index < index_count; ++index) {
                if (index_stride == 2) {
                    indices[index] = 0;
                    indices[index] = *reinterpret_cast<const std::uint16_t*>(data + index * index_stride);
                } else if (index_stride == 4) {
                    indices[index] = *reinterpret_cast<const std::uint32_t*>(data + index * index_stride);
                }
            }

            std::vector<vertex> vertices;
            for (const auto& [name, attribute] : gltf_primitive["attributes"].items()) {
                if (name != "POSITION" && name != "NORMAL" && name != "TEXCOORD_0") {
                    continue;
                }

                const std::size_t accessor_index = attribute;
                const auto& gltf_accessor = gltf_accessors[accessor_index];

                const std::size_t count = gltf_accessor["count"];
                const std::size_t byte_offset = gltf_accessor.contains("byteOffset") ?
                    (unsigned int)gltf_accessor["byteOffset"] : 0u;

                const std::size_t buffer_view_index = gltf_accessor["bufferView"];
                const auto& gltf_buffer_view = gltf_buffer_views[buffer_view_index];

                const std::size_t buffer_index = gltf_buffer_view["buffer"];
                const std::size_t buffer_offset = gltf_buffer_view.contains("byteOffset") ?
                    (unsigned int)gltf_buffer_view["byteOffset"] : 0u;
                const auto& buffer = buffers[buffer_index];

                if (vertices.size() < count) {
                    vertices.resize(count);
                }

                if (name == "POSITION") {
                    const std::size_t buffer_stride = gltf_buffer_view.contains("byteStride") ?
                        (unsigned int)gltf_buffer_view["byteStride"] : sizeof(vec3f); // TODO: Calculate byte stride!

                    const std::uint8_t* data = buffer.data() + byte_offset + buffer_offset;
                    for (std::size_t index{ 0 }; index < count; ++index) {
                        const auto ptr = data + index * buffer_stride;
                        vertices[index].position = *reinterpret_cast<const vec3f*>(ptr);
                    }
                } else if (name == "NORMAL") {
                    const std::size_t buffer_stride = gltf_buffer_view.contains("byteStride") ?
                        (unsigned int)gltf_buffer_view["byteStride"] : sizeof(vec3f); // TODO: Calculate byte stride!

                    const std::uint8_t* data = buffer.data() + byte_offset + buffer_offset;
                    for (std::size_t index{ 0 }; index < count; ++index) {
                        const auto ptr = data + index * buffer_stride;
                        vertices[index].normal = *reinterpret_cast<const vec3f*>(ptr);
                    }
                } else if (name == "TEXCOORD_0") {
                    const std::size_t buffer_stride = gltf_buffer_view.contains("byteStride") ?
                        (unsigned int)gltf_buffer_view["byteStride"] : sizeof(vec2f); // TODO: Calculate byte stride!

                    const std::uint8_t* data = buffer.data() + byte_offset + buffer_offset;
                    for (std::size_t index{ 0 }; index < count; ++index) {
                        const auto ptr = data + index * buffer_stride;
                        vertices[index].texcoord = *reinterpret_cast<const vec2f*>(ptr);
                    }
                }
            }

            const std::string mesh_name = format("mesh_{}_{}", mesh_index, primitive_index);
            const auto mesh_filename = mesh_name + ".msh";
            const auto mesh_metadata_filename = mesh_filename + ".meta";

            {
                fobstream stream{ (data_path / mesh_filename).string() };
                mesh::save(stream, vertices, indices);
            }

            json jmesh_metadata;
            if (std::filesystem::exists(data_path / mesh_metadata_filename)) {
                std::ifstream mesh_metadata_stream{ data_path / mesh_metadata_filename };
                mesh_metadata_stream >> jmesh_metadata;
            } else {
                std::ofstream mesh_metadata_stream{ data_path / mesh_metadata_filename };
                jmesh_metadata["uuid"] = uuid::generate().to_string();
                mesh_metadata_stream << jmesh_metadata;
            }

            editor::import((data_path / mesh_filename).string());

            meshes[mesh_index].push_back(jmesh_metadata["uuid"]);
            primitive_index++;
        }

        print("mesh: {}/{}\n", mesh_index, gltf_meshes.size());
        mesh_index++;
    }

    // 4. Convert nodes hierarchy to flat array.
    const std::size_t scene_index = gltf["scene"];
    const auto& gltf_scene = gltf_scenes[scene_index];

    json jentities{ json::array() };
    for (const auto& gltf_node_index : gltf_scene["nodes"]) {
        const std::size_t node_index = gltf_node_index;
        const auto& gltf_node = gltf_nodes[node_index];

        jentities.push_back(parse_node(gltf_meshes, meshes, gltf_materials, materials, gltf_nodes, gltf_node));
    }

    // 5. Save as prefab.
    json jprefab = {
        { "entities", jentities }
    };

    const auto cbor = json::to_cbor(jprefab);

    output.write(prefab::magic_number);
    output.write<std::uint32_t>(cbor.size());
    output.write(&cbor[0], cbor.size());
}
