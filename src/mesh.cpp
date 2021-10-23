#include <rabbit/mesh.hpp>
#include <rabbit/config.hpp>
#include <rabbit/graphics.hpp>

#include <meshoptimizer.h>

#include <fstream>
#include <sstream>
#include <vector>

using namespace rb;

// TODO: Vertex quantization
// TODO: Add more formats (.dae, .gltf)

std::shared_ptr<mesh> mesh::load(const std::string& filename, json& metadata) {
    std::ifstream istream{ filename, std::ios::in };
    RB_ASSERT(istream.is_open(), "Cannot open file");

    std::vector<vertex> vertices;
    std::vector<vec3f> positions;
    std::vector<vec2f> texcoords;
    std::vector<vec3f> normals;

    std::string line;
    while (std::getline(istream, line)) {
        std::istringstream iss{ line };
        std::vector<std::string> results{ std::istream_iterator<std::string>{ iss }, std::istream_iterator<std::string>{} };

        if (!results.empty()) {
            if (results[0] == "v") {
                positions.push_back({
                    static_cast<float>(std::atof(results[1].c_str())),
                    static_cast<float>(std::atof(results[2].c_str())),
                    static_cast<float>(std::atof(results[3].c_str()))
                });
            } else if (results[0] == "vn") {
                normals.push_back({
                    static_cast<float>(std::atof(results[1].c_str())),
                    static_cast<float>(std::atof(results[2].c_str())),
                    static_cast<float>(std::atof(results[3].c_str()))
                });
            } else if (results[0] == "vt") {
                texcoords.push_back({
                    static_cast<float>(std::atof(results[1].c_str())),
                    1.0f - static_cast<float>(std::atof(results[2].c_str())),
                });
            } else if (results[0] == "f") {
                int a, b, c;
                for (int i = 1; i < 4; ++i) {
                    sscanf(results[i].c_str(), "%d/%d/%d", &a, &b, &c);

                    vertices.push_back({
                        positions[a - 1],
                        texcoords[b - 1],
                        normals[c - 1]
                    });
                }
            }
        }
    }

    auto remap = std::make_unique<std::uint32_t[]>(vertices.size());
    const auto vertex_size = meshopt_generateVertexRemap(remap.get(), nullptr, vertices.size(), vertices.data(), vertices.size(), sizeof(vertex));

    auto indices = std::make_unique<std::uint32_t[]>(vertices.size());
    meshopt_remapIndexBuffer(indices.get(), nullptr, vertices.size(), remap.get());

    auto indexed_vertices = std::make_unique<vertex[]>(vertex_size);
    meshopt_remapVertexBuffer(indexed_vertices.get(), vertices.data(), vertices.size(), sizeof(vertex), remap.get());

    meshopt_optimizeVertexCache(indices.get(), indices.get(), vertices.size(), vertex_size);

    meshopt_optimizeOverdraw(indices.get(), indices.get(), vertices.size(), &indexed_vertices.get()->position.x, vertex_size, sizeof(vertex), 1.05f);
    meshopt_optimizeVertexFetch(indexed_vertices.get(), indices.get(), vertices.size(), indexed_vertices.get(), vertex_size, sizeof(vertex));

    mesh_desc desc;
    desc.vertices = span<const vertex>{ indexed_vertices.get(), vertex_size };
    desc.indices = span<const std::uint32_t>{ indices.get(), vertices.size() };
    return graphics::make_mesh(desc);
}

span<const vertex> mesh::vertices() const {
	return _vertices;
}

span<const std::uint32_t> mesh::indices() const {
	return _indices;
}

mesh::mesh(const mesh_desc& desc)
	: _vertices(desc.vertices.begin(), desc.vertices.end())
	, _indices(desc.indices.begin(), desc.indices.end()) {
}
