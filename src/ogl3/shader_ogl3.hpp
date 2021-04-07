#pragma once 

#include <rabbit/shader.hpp>

#include <GL/glew.h>

namespace rb {
    class shader_ogl3 : public shader {
    public:
        shader_ogl3(const shader_desc& desc);

        ~shader_ogl3();
    
        GLuint id() const;
        
    private:
        GLuint _program{ 0 };
    };
}
