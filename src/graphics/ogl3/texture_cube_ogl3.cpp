#include "texture_cube_ogl3.hpp"

#include <map>

using namespace rb;

static std::map<texture_format, GLenum> formats = {
	{ texture_format::r8, GL_RED },
	{ texture_format::rg8, GL_RG },
	{ texture_format::rgba8, GL_RGBA },
	{ texture_format::d24s8, GL_DEPTH_STENCIL }
};

static std::map<texture_format, GLint> internal_formats = {
	{ texture_format::r8, GL_R8 },
	{ texture_format::rg8, GL_RG8 },
	{ texture_format::rgba8, GL_RGBA8 },
	{ texture_format::d24s8, GL_DEPTH24_STENCIL8 }
};

static std::map<texture_filter, GLint> filters = {
	{ texture_filter::nearest, GL_NEAREST },
	{ texture_filter::linear, GL_LINEAR },
};

static std::map<texture_filter, GLint> mipmaps_filters = {
	{ texture_filter::nearest, GL_NEAREST_MIPMAP_NEAREST },
	{ texture_filter::linear, GL_LINEAR_MIPMAP_LINEAR },
};

static std::map<texture_wrap, GLint> wraps = {
	{ texture_wrap::clamp, GL_CLAMP_TO_EDGE },
	{ texture_wrap::repeat, GL_REPEAT },
};

static std::map<texture_format, int> bytes_per_pixels = {
	{ texture_format::r8, 1 },
	{ texture_format::rg8, 2 },
	{ texture_format::rgba8, 4 },
	{ texture_format::d24s8, 4 }
};

static std::map<texture_format, bool> depth_formats = {
	{ texture_format::r8, false },
	{ texture_format::rg8, false },
	{ texture_format::rgba8, false },
	{ texture_format::d24s8, true }
};

static std::map<texture_cube_face, GLint> texture_cube_faces = {
	{ texture_cube_face::positive_x, GL_TEXTURE_CUBE_MAP_POSITIVE_X },
	{ texture_cube_face::negative_x, GL_TEXTURE_CUBE_MAP_NEGATIVE_X },
	{ texture_cube_face::positive_y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y },
	{ texture_cube_face::negative_y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y },
	{ texture_cube_face::positive_z, GL_TEXTURE_CUBE_MAP_POSITIVE_Z },
	{ texture_cube_face::negative_z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z },
};

texture_cube_ogl3::texture_cube_ogl3(const texture_cube_desc& desc)
    : texture_cube(desc) {
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, desc.mipmaps ? mipmaps_filters.at(desc.filter) : filters.at(desc.filter));
	glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, filters.at(desc.filter));
	glTextureParameteri(_id, GL_TEXTURE_WRAP_S, wraps.at(desc.wrap));
	glTextureParameteri(_id, GL_TEXTURE_WRAP_T, wraps.at(desc.wrap));
	glTextureParameteri(_id, GL_TEXTURE_WRAP_R, wraps.at(desc.wrap));

	glTextureStorage2D(_id, desc.mipmaps, GL_RGBA8, desc.size.x, desc.size.y);

	for (const auto& [face, data] : desc.data) {
		glTextureSubImage3D(_id, 0, 0, 0, (int)face, desc.size.x, desc.size.y, 1, formats.at(desc.format), GL_UNSIGNED_BYTE, data.data());
	}

    if (desc.generate_mipmap) {
		glGenerateTextureMipmap(_id);
    }

	if (desc.is_render_target) {
		glCreateFramebuffers(1, &_framebuffer_id);
	}
}

texture_cube_ogl3::~texture_cube_ogl3() {
	glDeleteTextures(1, &_id);
	glDeleteFramebuffers(1, &_framebuffer_id);
}

void texture_cube_ogl3::update(texture_cube_face face, const span<const std::uint8_t>& pixels, const vec4i& rect) {
	glTextureSubImage3D(_id, 0, rect.x, rect.y, 0, rect.z, rect.w, 1, formats.at(format()), GL_UNSIGNED_BYTE, pixels.data());
}

GLuint texture_cube_ogl3::id() const {
	return _id;
}

GLuint texture_cube_ogl3::framebuffer_id() const {
	return _framebuffer_id;
}
