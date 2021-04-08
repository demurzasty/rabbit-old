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
	glGenTextures(1, &_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, desc.mipmaps ? mipmaps_filters.at(desc.filter) : filters.at(desc.filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filters.at(desc.filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wraps.at(desc.wrap));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wraps.at(desc.wrap));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wraps.at(desc.wrap));

    for (const auto& [face, face_id] : texture_cube_faces) {
        const auto data = desc.data.find(face) != desc.data.end() ? desc.data.at(face) : gsl::span<const std::uint8_t>{}; 
        glTexImage2D(face_id, 0, internal_formats.at(desc.format), desc.size.x, desc.size.y, 0, formats.at(desc.format), GL_UNSIGNED_BYTE, !data.empty() ? data.data() : nullptr);
    }

    if (desc.generate_mipmap) {
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

	if (desc.is_render_target) {
        for (const auto& [face, face_id] : texture_cube_faces) {
            GLuint framebuffer_id;
            glGenFramebuffers(1, &framebuffer_id);
		    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
		    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face_id, _id, 0);
			_framebuffer_ids[face] = framebuffer_id;
        }
        
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

texture_cube_ogl3::~texture_cube_ogl3() {
	glDeleteTextures(1, &_id);

    for (const auto& [face, framebuffer_id] : _framebuffer_ids) {
	    glDeleteFramebuffers(1, &framebuffer_id);
    }
}

void texture_cube_ogl3::update(texture_cube_face face, const span<const std::uint8_t>& pixels, const vec4i& rect) {
	glBindTexture(GL_TEXTURE_CUBE_MAP, _id);
	glTexSubImage2D(texture_cube_faces.at(face), 0, rect.x, rect.y, rect.z, rect.w, formats.at(format()), GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

GLuint texture_cube_ogl3::id() const {
	return _id;
}

GLuint texture_cube_ogl3::framebuffer_id(texture_cube_face face) const {
	return _framebuffer_ids.at(face);
}
