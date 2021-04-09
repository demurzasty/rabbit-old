#pragma once 

#include <rabbit/graphics/mesh.hpp>

#include <GL/glew.h>

namespace rb {
    class mesh_ogl3 : public mesh {
    public:
        mesh_ogl3(const mesh_desc& desc);

        ~mesh_ogl3();
        
        GLuint id() const;

    private:
        GLuint _id;
    };
}