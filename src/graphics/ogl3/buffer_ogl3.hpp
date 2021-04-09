#pragma once 

#include <rabbit/graphics/buffer.hpp>

#include <GL/glew.h>

namespace rb {
    class buffer_ogl3 : public buffer {
    public:
        buffer_ogl3(const buffer_desc& desc);

        ~buffer_ogl3();

        void* map() override;

        void unmap() override;

        GLuint id() const;

    protected:
        void update(const void* data, std::size_t size) override;

    private:
        GLuint _id;
        GLenum _type;
    };
}