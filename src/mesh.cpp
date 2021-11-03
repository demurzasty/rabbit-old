#include <rabbit/mesh.hpp>
#include <rabbit/config.hpp>
#include <rabbit/graphics.hpp>
#include <rabbit/bstream.hpp>
#include <rabbit/math.hpp>

#include <meshoptimizer.h>
#include <QuickHull.hpp>

#include <fstream>
#include <sstream>
#include <vector>

using namespace rb;

// TODO: Vertex quantization
// TODO: Add more formats (.gltf)

static bspheref calculate_bsphere(const span<const vertex>& vertices) {
    bspheref bsphere{ vec3f::zero(), 0.0f };

    // 1. Calculate center of sphere
    for (auto& vertex : vertices) {
        bsphere.position = bsphere.position + vertex.position;
    }
    bsphere.position = bsphere.position / static_cast<float>(vertices.size());

    // 2. Calculate maximum radius
    for (auto& vertex : vertices) {
        const auto distance = length(vertex.position - bsphere.position);
        if (distance > bsphere.radius) {
            bsphere.radius = distance;
        }
    }

    return bsphere;
}

std::shared_ptr<mesh> mesh::load(bstream& stream) {
    std::uint32_t vertex_count;
    stream.read(vertex_count);
    const auto vertices = std::make_unique<vertex[]>(vertex_count);
    stream.read(vertices.get(), vertex_count * sizeof(vertex));

    std::uint32_t index_count;
    stream.read(index_count);
    const auto indices = std::make_unique<std::uint32_t[]>(index_count);
    stream.read(indices.get(), index_count * sizeof(std::uint32_t));

    std::uint32_t triangle_count;
    stream.read(triangle_count);
    const auto triangles = std::make_unique<trianglef[]>(triangle_count);
    stream.read(triangles.get(), triangle_count * sizeof(trianglef));

    bspheref bsphere;
    //stream.read(bsphere);

    mesh_desc desc;
    desc.vertices = { vertices.get(), vertex_count };
    desc.indices = { indices.get(), index_count };
    desc.convex_hull = { triangles.get(), triangle_count };
   // desc.bsphere = bsphere;
    return graphics::make_mesh(desc);
}

void mesh::import(const std::string& input, bstream& output, const json& metadata) {
    std::ifstream istream{ input, std::ios::in };
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

    const auto remap = std::make_unique<std::uint32_t[]>(vertices.size());
    const auto vertex_size = meshopt_generateVertexRemap(remap.get(), nullptr, vertices.size(), vertices.data(), vertices.size(), sizeof(vertex));

    const auto indices = std::make_unique<std::uint32_t[]>(vertices.size());
    meshopt_remapIndexBuffer(indices.get(), nullptr, vertices.size(), remap.get());

    const auto indexed_vertices = std::make_unique<vertex[]>(vertex_size);
    meshopt_remapVertexBuffer(indexed_vertices.get(), vertices.data(), vertices.size(), sizeof(vertex), remap.get());

    meshopt_optimizeVertexCache(indices.get(), indices.get(), vertices.size(), vertex_size);

    meshopt_optimizeOverdraw(indices.get(), indices.get(), vertices.size(), &indexed_vertices.get()->position.x, vertex_size, sizeof(vertex), 1.05f);
    meshopt_optimizeVertexFetch(indexed_vertices.get(), indices.get(), vertices.size(), indexed_vertices.get(), vertex_size, sizeof(vertex));

    quickhull::QuickHull<float> quickhull;
    auto hull = quickhull.getConvexHull(&positions[0].x, positions.size(), true, false);
    auto hull_indices = hull.getIndexBuffer();
    auto hull_vertices = hull.getVertexBuffer();

    std::vector<vec3f> convex_hull;
    for (auto& index : hull_indices) {
        convex_hull.push_back({
            hull_vertices[index].x,
            hull_vertices[index].y,
            hull_vertices[index].z,
        });
    }

    const auto bsphere = calculate_bsphere({ indexed_vertices.get(), vertex_size });

    const auto triangle_count = convex_hull.size() / 3;

    output.write(mesh::magic_number);
    output.write<std::uint32_t>(vertex_size);
    output.write(indexed_vertices.get(), vertex_size * sizeof(vertex));
    output.write<std::uint32_t>(vertices.size());
    output.write(indices.get(), vertices.size() * sizeof(std::uint32_t));
    output.write<std::uint32_t>(triangle_count); // in triangle count
    output.write(convex_hull.data(), triangle_count * sizeof(trianglef));
    output.write(bsphere);
}

std::shared_ptr<mesh> mesh::make_box(const vec3f& extent, const vec2f& uv_scale) {
    vertex vertices[24]{
        { { -extent.x, extent.y, extent.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -extent.x, -extent.y, extent.z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { { extent.x, -extent.y, extent.z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { { extent.x, extent.y, extent.z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },

        { { extent.x, extent.y, extent.z }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { extent.x, -extent.y, extent.z }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { extent.x, -extent.y, -extent.z }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { extent.x, extent.y, -extent.z }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },

        { { extent.x, extent.y, -extent.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { { extent.x, -extent.y, -extent.z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
        { { -extent.x, -extent.y, -extent.z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
        { { -extent.x, extent.y, -extent.z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },

        { { -extent.x, extent.y, -extent.z }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -extent.x, -extent.y, -extent.z }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -extent.x, -extent.y, extent.z }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } },
        { { -extent.x, extent.y, extent.z }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } },

        { { -extent.x, extent.y, -extent.z }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { -extent.x, extent.y, extent.z }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
        { { extent.x, extent.y, extent.z }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
        { { extent.x, extent.y, -extent.z }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },

        { { -extent.x, -extent.y, extent.z }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
        { { -extent.x, -extent.y, -extent.z }, { 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } },
        { { extent.x, -extent.y, -extent.z }, { 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } },
        { { extent.x, -extent.y, extent.z }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
    };

    for (auto& vertex : vertices) {
        vertex.texcoord.x *= uv_scale.x;
        vertex.texcoord.y *= uv_scale.y;
    }

    const std::uint32_t indices[36]{
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4,

        8, 9, 10,
        10, 11, 8,

        12, 13, 14,
        14, 15, 12,

        16, 17, 18,
        18, 19, 16,

        20, 21, 22,
        22, 23, 20
    };

    mesh_desc desc;
    desc.vertices = vertices;
    desc.indices = indices;
    return graphics::make_mesh(desc);
}

std::shared_ptr<mesh> mesh::make_sphere(std::size_t stacks, std::size_t slices, float radius) {
    auto vertices = std::make_unique<vertex[]>((slices + 1) * (stacks + 1));
    auto indices = std::make_unique<std::uint32_t[]>(slices * slices * 6);

    float phi, theta;
    float dphi = pi<float>() / stacks;
    float dtheta = (2.0f * pi<float>()) / slices;
    float x, y, z, sc;
    unsigned int index = 0;
    int k;

    for (int stack = 0u; stack <= stacks; stack++) {
        phi = pi<float>() * 0.5f - stack * dphi;
        y = std::sin(phi) * radius;
        sc = -std::cos(phi);

        for (auto slice = 0u; slice <= slices; slice++) {
            theta = slice * dtheta;
            x = sc * std::sin(theta) * radius;
            z = sc * std::cos(theta) * radius;

            vertices[index] = { x, y, z };

            index++;
        }
    }

    index = 0;
    k = slices + 1;

    for (auto stack = 0u; stack < stacks; stack++) {
        for (auto slice = 0u; slice < slices; slice++) {
            indices[index++] = (stack + 0) * k + slice;
            indices[index++] = (stack + 1) * k + slice;
            indices[index++] = (stack + 0) * k + slice + 1;

            indices[index++] = (stack + 0) * k + slice + 1;
            indices[index++] = (stack + 1) * k + slice;
            indices[index++] = (stack + 1) * k + slice + 1;
        }
    }

    mesh_desc desc;
    desc.vertices = { vertices.get(), (slices + 1) * (stacks + 1) };
    desc.indices = { indices.get(), slices * slices * 6 };
    desc.bsphere = { { 0.0f, 0.0f, 0.0f }, radius };
    return graphics::make_mesh(desc);
}

span<const vertex> mesh::vertices() const {
	return _vertices;
}

span<const std::uint32_t> mesh::indices() const {
	return _indices;
}

span<const trianglef> mesh::convex_hull() const {
    return _convex_hull;
}

const bspheref& mesh::bsphere() const {
    return _bsphere;
}

mesh::mesh(const mesh_desc& desc)
	: _vertices(desc.vertices.begin(), desc.vertices.end())
	, _indices(desc.indices.begin(), desc.indices.end())
    , _convex_hull(desc.convex_hull.begin(), desc.convex_hull.end())
    , _bsphere(desc.bsphere.has_value() ? desc.bsphere.value() : calculate_bsphere(desc.vertices)) {
    RB_ASSERT(!_vertices.empty(), "No vertices has been provided for mesh.");
    RB_ASSERT(!_indices.empty(), "No indices has been provided for mesh.");
}
