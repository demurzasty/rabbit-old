#include <rabbit/editor/importers/mesh_importer.hpp>
#include <rabbit/engine/graphics/mesh.hpp>

#include <meshoptimizer.h>
#include <QuickHull.hpp>

using namespace rb;

static void load_mesh(ibstream& input, std::vector<vertex>& vertices, std::vector<std::uint32_t>& indices, std::vector<vec3f>& positions) {
    vertices.clear();
    positions.clear();
    indices.clear();

    std::uint32_t vertex_count;
    input.read(vertex_count);
    vertices.resize(vertex_count);
    input.read(&vertices[0], vertex_count * sizeof(vertex));

    std::uint32_t index_count;
    input.read(index_count);
    indices.resize(index_count);
    input.read(&indices[0], index_count * sizeof(std::uint32_t));

    positions.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        positions.push_back(vertex.position);
    }
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

void mesh_importer::import(ibstream& input, obstream& output, const json& metadata) {
    std::vector<vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<vec3f> positions;
    load_mesh(input, vertices, indices, positions);

    optimize(vertices, indices);
    optimize_vertices(vertices, indices);

    // level of details indices (excluding base indices)
    std::array<std::vector<std::uint32_t>, 4> lods{
        simplify(vertices, indices, 0.8f, 0.05f),
        simplify(vertices, indices, 0.4f, 0.05f),
        simplify(vertices, indices, 0.2f, 0.05f),
        simplify(vertices, indices, 0.075f, 0.05f),
    };

    for (auto& lod : lods) {
        optimize(vertices, lod);
    }

    // build clusters
    const auto clustered_data = mesh_utils::calculate_clusters(vertices, indices, { 0, static_cast<std::uint32_t>(indices.size()) });

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

    const auto bsphere = mesh_utils::calculate_bsphere(vertices);
    const auto bbox = mesh_utils::calculate_bbox(vertices);

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

    output.write<std::uint32_t>(clustered_data.clusters.size());
    output.write(clustered_data.clusters.data(), clustered_data.clusters.size() * sizeof(mesh_lod));
    output.write<std::uint32_t>(clustered_data.indices.size());
    output.write(clustered_data.indices.data(), clustered_data.indices.size() * sizeof(std::uint32_t));

    output.write<std::uint32_t>(convex_hull.size() / 3); // in triangle count
    output.write(convex_hull.data(), convex_hull.size() * sizeof(vec3f));
    output.write(bsphere);
    output.write(bbox);
}
