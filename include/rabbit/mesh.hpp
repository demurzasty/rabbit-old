#pragma once 

#include "buffer.hpp"
#include "vertex_desc.hpp"

#include <memory>
#include <vector>

namespace rb {
    struct mesh_desc {
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

        const vertex_desc& vertex_desc();

        const std::shared_ptr<buffer>& vertex_buffer();

        const std::shared_ptr<buffer>& index_buffer();

    private:
        rb::vertex_desc _vertex_desc;
        std::shared_ptr<buffer> _vertex_buffer;
        std::shared_ptr<buffer> _index_buffer;
    };
}
