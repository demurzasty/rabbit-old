#include <rabbit/engine/graphics/mesh.hpp>
#include <rabbit/engine/core/config.hpp>
#include <rabbit/engine/graphics/graphics.hpp>
#include <rabbit/engine/core/bstream.hpp>
#include <rabbit/engine/math/math.hpp>

#include <meshoptimizer.h>
#include <QuickHull.hpp>

#include <fstream>
#include <sstream>
#include <vector>

using namespace rb;

bspheref mesh_utils::calculate_bsphere(const span<const vertex>& vertices) {
    bspheref bsphere{ vec3f::zero(), 0.0f };

    // 1. Calculate center of sphere
    for (const auto& vertex : vertices) {
        bsphere.position = bsphere.position + vertex.position;
    }
    bsphere.position = bsphere.position / static_cast<float>(vertices.size());

    // 2. Calculate maximum radius
    for (const auto& vertex : vertices) {
        const auto distance = length(vertex.position - bsphere.position);
        if (distance > bsphere.radius) {
            bsphere.radius = distance;
        }
    }

    return bsphere;
}

bboxf mesh_utils::calculate_bbox(const span<const vertex>& vertices) {
    constexpr auto fmax = std::numeric_limits<float>::max();
    constexpr auto fmin = std::numeric_limits<float>::min();

    bboxf bbox{ vec3f{ fmax, fmax, fmax }, vec3f{ fmin, fmin, fmin } };

    for (const auto& vertex : vertices) {
        bbox.min.x = std::min(bbox.min.x, vertex.position.x);
        bbox.min.y = std::min(bbox.min.y, vertex.position.y);
        bbox.min.y = std::min(bbox.min.z, vertex.position.z);

        bbox.max.x = std::max(bbox.max.x, vertex.position.x);
        bbox.max.y = std::max(bbox.max.y, vertex.position.y);
        bbox.max.z = std::max(bbox.max.z, vertex.position.z);
    }

    return bbox;
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

    mesh_clustered_data clustered_data;

    std::uint32_t cluster_count;
    stream.read(cluster_count);
    clustered_data.clusters.resize(cluster_count);
    stream.read(clustered_data.clusters.data(), cluster_count * sizeof(mesh_lod));

    std::uint32_t clusters_index_count;
    stream.read(clusters_index_count);
    clustered_data.indices.resize(clusters_index_count);
    stream.read(clustered_data.indices.data(), clusters_index_count * sizeof(std::uint32_t));

    std::uint32_t triangle_count;
    stream.read(triangle_count);
    const auto triangles = std::make_unique<trianglef[]>(triangle_count);
    stream.read(triangles.get(), triangle_count * sizeof(trianglef));

    bspheref bsphere;
    stream.read(bsphere);

    bboxf bbox;
    stream.read(bbox);

    mesh_desc desc;
    desc.vertices = { vertices.get(), vertex_count };
    desc.indices = { indices.get(), total_index_count };
    desc.lods = { lods.get(), lod_count };
    desc.clustered_data = clustered_data;
    desc.convex_hull = { triangles.get(), triangle_count };
    desc.bsphere = bsphere;
    desc.bbox = bbox;
    return graphics::make_mesh(desc);
}

mesh_clustered_data mesh_utils::calculate_clusters(const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const mesh_lod& lod) {
    const auto max_vertices = 64u;
    const auto max_triangles = 124u;
    const auto cone_weight = 0.0f;
    const auto max_meshlets = meshopt_buildMeshletsBound(lod.size, max_vertices, max_triangles);
    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
    std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);
    meshlets.resize(meshopt_buildMeshlets(&meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &indices[lod.offset], lod.size, &vertices[0].position.x, vertices.size(), sizeof(vertex), max_vertices, max_triangles, cone_weight));

    if (meshlets.size()) {
        const auto& last = meshlets.back();

        // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
        meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
        meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    }

    std::vector<mesh_lod> clusters;
    std::vector<std::uint32_t> clusters_indices;

    std::uint32_t offset{ 0 };
    for (const auto& meshlet : meshlets) {
        clusters.push_back({
            offset,
            meshlet.triangle_count * 3
        });

        for (auto i = 0u; i < meshlet.triangle_count; ++i) {
            const auto vertex_index1 = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + i * 3]];
            const auto vertex_index2 = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + i * 3 + 1]];
            const auto vertex_index3 = meshlet_vertices[meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + i * 3 + 2]];

            clusters_indices.push_back(vertex_index1);
            clusters_indices.push_back(vertex_index2);
            clusters_indices.push_back(vertex_index3);
        }

        offset += meshlet.triangle_count * 3;
    }

    return { clusters, clusters_indices };
}

void mesh_utils::save(obstream& stream, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) {
    stream.write<std::uint32_t>(vertices.size());
    stream.write(vertices.data(), vertices.size_bytes());
    stream.write<std::uint32_t>(indices.size());
    stream.write(indices.data(), indices.size_bytes());
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

const mesh_clustered_data& mesh::clustered_data() const {
    return _clustered_data;
}

span<const trianglef> mesh::convex_hull() const {
    return _convex_hull;
}

const bspheref& mesh::bsphere() const {
    return _bsphere;
}

const bboxf& mesh::bbox() const {
    return _bbox;
}

mesh::mesh(const mesh_desc& desc)
	: _vertices(desc.vertices.begin(), desc.vertices.end())
	, _indices(desc.indices.begin(), desc.indices.end())
    , _lods(desc.lods.begin(), desc.lods.end())
    , _clustered_data(desc.clustered_data ? *desc.clustered_data : mesh_utils::calculate_clusters(desc.vertices, desc.indices, !_lods.empty() ? _lods.front() : mesh_lod{ 0u, static_cast<std::uint32_t>(desc.indices.size()) }))
    , _convex_hull(desc.convex_hull.begin(), desc.convex_hull.end())
    , _bsphere(desc.bsphere ? *desc.bsphere : mesh_utils::calculate_bsphere(desc.vertices))
    , _bbox(desc.bbox ? *desc.bbox : mesh_utils::calculate_bbox(desc.vertices)) {
    RB_ASSERT(!_vertices.empty(), "No vertices has been provided for mesh.");
    RB_ASSERT(!_indices.empty(), "No indices has been provided for mesh.");
}
