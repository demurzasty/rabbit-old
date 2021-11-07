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

std::shared_ptr<mesh> mesh::load(ibstream& stream) {
    std::uint32_t vertex_count;
    stream.read(vertex_count);
    const auto vertices = std::make_unique<vertex[]>(vertex_count);
    stream.read(vertices.get(), vertex_count * sizeof(vertex));

    std::uint32_t total_index_count;
    stream.read(total_index_count);
    const auto indices = std::make_unique<std::uint32_t[]>(total_index_count);
    stream.read(indices.get(), total_index_count * sizeof(std::uint32_t));

    std::uint32_t lod_count;
    stream.read(lod_count);
    const auto lods = std::make_unique<mesh_lod[]>(lod_count);
    stream.read(lods.get(), lod_count * sizeof(mesh_lod));

    std::uint32_t triangle_count;
    stream.read(triangle_count);
    const auto triangles = std::make_unique<trianglef[]>(triangle_count);
    stream.read(triangles.get(), triangle_count * sizeof(trianglef));

    bspheref bsphere;
    stream.read(bsphere);

    mesh_desc desc;
    desc.vertices = { vertices.get(), vertex_count };
    desc.indices = { indices.get(), total_index_count };
    desc.lods = { lods.get(), lod_count };
    desc.convex_hull = { triangles.get(), triangle_count };
    desc.bsphere = bsphere;
    return graphics::make_mesh(desc);
}

static void load_obj(ibstream& input, std::vector<vertex>& vertices, std::vector<std::uint32_t>& indices, std::vector<vec3f>& positions) {
    vertices.clear();
    positions.clear();
    std::vector<vertex> temporal_vertices;
    std::vector<vec2f> texcoords;
    std::vector<vec3f> normals;

    const auto getline = [](ibstream& stream, std::string& line) -> bool {
        line.clear();

        char c;
        while (!stream.eof()) {
            stream.read(c);

            if (c == '\n') {
                break;
            }

            line.push_back(c);
        }

        return !line.empty();
    };

    std::string line;
    while (getline(input, line)) {
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

                    temporal_vertices.push_back({
                        positions[a - 1],
                        texcoords[b - 1],
                        normals[c - 1]
                    });
                }
            }
        }
    }

    const auto remap = std::make_unique<std::uint32_t[]>(temporal_vertices.size());
    const auto vertex_count = meshopt_generateVertexRemap(remap.get(), nullptr, temporal_vertices.size(), temporal_vertices.data(), temporal_vertices.size(), sizeof(vertex));

    indices.resize(temporal_vertices.size());
    meshopt_remapIndexBuffer(indices.data(), nullptr, temporal_vertices.size(), remap.get());

    vertices.resize(vertex_count);
    meshopt_remapVertexBuffer(vertices.data(), temporal_vertices.data(), temporal_vertices.size(), sizeof(vertex), remap.get());

}

static void optimize(std::vector<vertex>& vertices, std::vector<std::uint32_t>& indices) {
    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
    meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &vertices[0].position.x, vertices.size(), sizeof(vertex), 1.05f);
}

static void optimize_vertices(std::vector<vertex>& vertices, std::vector<std::uint32_t>& indices) {
    meshopt_optimizeVertexFetch(vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(vertex));
}

static std::vector<std::uint32_t> simplify(std::vector<vertex>& vertices, const std::vector<std::uint32_t>& indices, float threshold, float target_error) {
    std::size_t target_index_count = static_cast<std::size_t>(indices.size() * threshold);

    std::vector<std::uint32_t> lod(indices.size());
    float lod_error = 0.f;

    std::size_t lod_size = meshopt_simplify(&lod[0], indices.data(), indices.size(), &vertices[0].position.x, vertices.size(), sizeof(vertex), target_index_count, target_error, &lod_error);
    lod.resize(lod_size);

    optimize(vertices, lod);
    return lod;
}

void mesh::import(ibstream& input, obstream& output, const json& metadata) {
    std::vector<vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<vec3f> positions;
    load_obj(input, vertices, indices, positions);

    optimize(vertices, indices);
    optimize_vertices(vertices, indices);

    // level of details indices (excluding base indices)
    std::array<std::vector<std::uint32_t>, 5> lods{
        simplify(vertices, indices, 0.8f, 0.05f),
        simplify(vertices, indices, 0.4f, 0.1f),
        simplify(vertices, indices, 0.2f, 0.2f),
        simplify(vertices, indices, 0.075f, 0.3f),
        simplify(vertices, indices, 0.025f, 0.5f),
    };

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

    const auto bsphere = calculate_bsphere(vertices);

    std::size_t total_index_count{ indices.size() };
    for (const auto& lod : lods) {
        total_index_count += lod.size();
    }

    output.write(mesh::magic_number);

    output.write<std::uint32_t>(vertices.size());
    output.write(vertices.data(), vertices.size() * sizeof(vertex));

    output.write<std::uint32_t>(total_index_count);
    output.write(indices.data(), indices.size() * sizeof(std::uint32_t)); // base indices
    for (const auto& lod : lods) {
        output.write(lod.data(), lod.size() * sizeof(std::uint32_t));
    }

    output.write<std::uint32_t>(lods.size() + 1); // including base indices
    output.write(mesh_lod{ 0u, static_cast<std::uint32_t>(indices.size()) });

    std::uint32_t offset{ static_cast<std::uint32_t>(indices.size()) };
    for (const auto& lod : lods) {
        const auto size = static_cast<std::uint32_t>(lod.size());
        output.write(mesh_lod{ offset, size });
        offset += size;
    }

    output.write<std::uint32_t>(convex_hull.size() / 3); // in triangle count
    output.write(convex_hull.data(), convex_hull.size() * sizeof(vec3f));
    output.write(bsphere);
}

void mesh::save(obstream& stream, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) {
    stream.write(mesh::magic_number);
    stream.write<std::uint32_t>(vertices.size());
    stream.write(vertices.data(), vertices.size_bytes());
    stream.write<std::uint32_t>(indices.size());
    stream.write(indices.data(), indices.size_bytes());
    stream.write<std::uint32_t>(0);
    // stream.write(convex_hull.data(), convex_hull.size() * sizeof(vec3f));
    stream.write(calculate_bsphere(vertices));
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

    mesh_lod lods[]{
        { 0, 36 }
    };

    mesh_desc desc;
    desc.vertices = vertices;
    desc.indices = indices;
    desc.lods = lods;
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

    mesh_lod lods[]{
        { 0u, static_cast<std::uint32_t>(slices * slices * 6) }
    };

    mesh_desc desc;
    desc.vertices = { vertices.get(), (slices + 1) * (stacks + 1) };
    desc.indices = { indices.get(), slices * slices * 6 };
    desc.lods = lods;
    desc.bsphere = { { 0.0f, 0.0f, 0.0f }, radius };
    return graphics::make_mesh(desc);
}

span<const vertex> mesh::vertices() const {
	return _vertices;
}

span<const std::uint32_t> mesh::indices() const {
	return _indices;
}

span<const mesh_lod> mesh::lods() const {
    return _lods;
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
    , _lods(desc.lods.begin(), desc.lods.end())
    , _convex_hull(desc.convex_hull.begin(), desc.convex_hull.end())
    , _bsphere(desc.bsphere.has_value() ? desc.bsphere.value() : calculate_bsphere(desc.vertices)) {
    RB_ASSERT(!_vertices.empty(), "No vertices has been provided for mesh.");
    RB_ASSERT(!_indices.empty(), "No indices has been provided for mesh.");
}
