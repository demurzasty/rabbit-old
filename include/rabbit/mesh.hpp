#pragma once 

#include "buffer.hpp"
#include "topology.hpp"
#include "vertex_desc.hpp"

#include <memory>
#include <vector>

namespace rb {
    struct mesh_desc {
        topology topology{ topology::triangles };
        vertex_desc vertex_desc;
        std::shared_ptr<buffer> vertex_buffer;
        std::shared_ptr<buffer> index_buffer;
    };

    class mesh {
    public:
        mesh(const mesh_desc& desc);

        virtual ~mesh() = default;

        mesh(const mesh&) = delete;
        mesh& operator=(const mesh&) = delete;

        topology topology() const;

        const vertex_desc& vertex_desc();

        const std::shared_ptr<buffer>& vertex_buffer();

        const std::shared_ptr<buffer>& index_buffer();

    private:
        rb::topology _topology;
        rb::vertex_desc _vertex_desc;
        std::shared_ptr<buffer> _vertex_buffer;
        std::shared_ptr<buffer> _index_buffer;
    };
}
