#pragma once 

#include <rabbit/graphics/texture.hpp>

#include <GL/glew.h>

namespace rb {
    class texture_ogl3 : public texture {
    public:
		texture_ogl3(const texture_desc& desc);

		~texture_ogl3();

		void update(const span<const std::uint8_t>& pixels, const vec4i& rect) override;

		GLuint id() const;

		GLuint framebuffer_id() const;

	private:
		GLuint _id = 0;
		GLuint _framebuffer_id = 0;
    };
}