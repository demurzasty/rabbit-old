#include <rabbit/model.hpp>
#include <rabbit/mesh.hpp>
#include <rabbit/material.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_RAPIDJSON 
#define TINYGLTF_USE_CPP14 
#include <tiny_gltf.h>

using namespace rb;

void model::import(const std::string& input, const std::string& output, const json& metadata) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    std::string err;
    std::string warn;
    const auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, input);

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return;
    }

    std::vector<std::shared_ptr<mesh>> meshes;
    std::vector<std::shared_ptr<material>> materials;

    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            const auto& index_accessor = model.accessors[primitive.indices];

            std::vector<std::uint32_t> indices(index_accessor.count);

            const auto& index_buffer_view = model.bufferViews[index_accessor.bufferView];
            const auto& index_buffer = model.buffers[index_buffer_view.buffer];
            const auto index_stride = index_accessor.ByteStride(index_buffer_view);
            printf("index_stride = %d\n", index_stride);

            const std::uint8_t* data = index_buffer.data.data() + (index_buffer_view.byteOffset + index_accessor.byteOffset);
            for (std::size_t index{ 0 }; index < index_accessor.count; ++index) {
                if (index_stride == 2) {
                    indices[index] = *reinterpret_cast<const std::uint16_t*>(data + index * index_stride);
                }
                else if (index_stride == 4) {
                    indices[index] = *reinterpret_cast<const std::uint32_t*>(data + index * index_stride);
                }
            }

            std::vector<vertex> vertices;

            for (const auto& [name, accessor_index] : primitive.attributes) {
                const auto& accessor = model.accessors[accessor_index];
                const auto& buffer_view = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[buffer_view.buffer];
                const auto stride = accessor.ByteStride(buffer_view);

                if (vertices.size() < accessor.count) {
                    vertices.resize(accessor.count);
                }

                if (name != "POSITION" && name != "NORMAL" && name != "TEXCOORD_0") {
                    continue;
                }

                if (name == "POSITION") {
                    const std::uint8_t* data = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
                    for (std::size_t index{ 0 }; index < accessor.count; ++index) {
                        const auto ptr = data + index * stride;
                        vertices[index].position = *reinterpret_cast<const vec3f*>(ptr);
                    }
                }

                if (name == "NORMAL") {
                    const std::uint8_t* data = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
                    for (std::size_t index{ 0 }; index < accessor.count; ++index) {
                        const auto ptr = data + index * stride;
                        vertices[index].normal = *reinterpret_cast<const vec3f*>(ptr);
                    }
                }

                if (name == "TEXCOORD_0") {
                    const std::uint8_t* data = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
                    for (std::size_t index{ 0 }; index < accessor.count; ++index) {
                        const auto ptr = data + index * stride;
                        vertices[index].texcoord = *reinterpret_cast<const vec2f*>(ptr);
                    }
                }
            }

            const auto& mat = model.materials[primitive.material];
            const auto& roughness_texture = model.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
            const auto& roughness_image = model.images[roughness_texture.source];

            print("image: {}\n", roughness_image.uri);

            //mesh_desc mesh_desc;
            //mesh_desc.vertices = vertices;
            //mesh_desc.indices = indices;
            //meshes.push_back(graphics::make_mesh(mesh_desc));

            //const auto& mat = model.materials[primitive.material];
            //const auto& texture = model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
            //const auto& image = model.images[texture.source];

            //texture_desc albedo_texture_desc;
            //albedo_texture_desc.data = image.image.data();
            //albedo_texture_desc.size = { (unsigned int)image.width, (unsigned int)image.height };
            //albedo_texture_desc.mipmaps = 1;

            //const auto& normal_texture = model.textures[mat.normalTexture.index];
            //const auto& normal_image = model.images[normal_texture.source];

            //texture_desc normal_texture_desc;
            //normal_texture_desc.data = normal_image.image.data();
            //normal_texture_desc.size = { (unsigned int)normal_image.width, (unsigned int)normal_image.height };
            //normal_texture_desc.mipmaps = 1;

            //const auto& roughness_texture = model.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
            //const auto& roughness_image = model.images[roughness_texture.source];

            //texture_desc roughness_texture_desc;
            //roughness_texture_desc.data = roughness_image.image.data();
            //roughness_texture_desc.size = { (unsigned int)roughness_image.width, (unsigned int)roughness_image.height };
            //roughness_texture_desc.mipmaps = 1;

            //material_desc material_desc;
            //material_desc.albedo_map = graphics::make_texture(albedo_texture_desc);
            //material_desc.normal_map = graphics::make_texture(normal_texture_desc);
            //material_desc.roughness_map = texture::make_one_color(color::black(), { 2, 2 });
            //material_desc.metallic_map = texture::make_one_color(color::black(), { 2, 2 });
            //material_desc.emissive_map = texture::make_one_color(color::black(), { 2, 2 });
            //material_desc.ambient_map = texture::make_one_color(color::white(), { 2, 2 });
            //materials.push_back(graphics::make_material(material_desc));
        }
    }
}
