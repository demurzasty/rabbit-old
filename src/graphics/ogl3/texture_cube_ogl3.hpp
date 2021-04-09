#pragma once 

#include <rabbit/graphics/texture_cube.hpp>

#include <GL/glew.h>

namespace rb {
    class texture_cube_ogl3 : public texture_cube {
    public:
		texture_cube_ogl3(const texture_cube_desc& desc);

		~texture_cube_ogl3();

		void update(texture_cube_face face, const span<const std::uint8_t>& pixels, const vec4i& rect) override;

		GLuint id() const;

		GLuint framebuffer_id() const;

	private:
		GLuint _id{ 0 };
		GLuint _framebuffer_id{ 0 };
    };
}
