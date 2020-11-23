#pragma once 

#include <rabbit/buffer.hpp>

#include <GL/glew.h>

namespace rb {
    class buffer_ogl3 : public buffer {
    public:
        buffer_ogl3(const buffer_desc& desc);

        ~buffer_ogl3();

        GLuint id() const;

    private:
        GLuint _id;
    };
}