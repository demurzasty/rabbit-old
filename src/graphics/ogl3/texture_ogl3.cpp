#include "texture_ogl3.hpp"

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

texture_ogl3::texture_ogl3(const texture_desc& desc)
    : texture(desc) {
	glCreateTextures(GL_TEXTURE_2D, 1, &_id);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, filters.at(desc.filter));
	glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, filters.at(desc.filter));
	glTextureParameteri(_id, GL_TEXTURE_WRAP_S, wraps.at(desc.wrap));
	glTextureParameteri(_id, GL_TEXTURE_WRAP_T, wraps.at(desc.wrap));

	glTextureStorage2D(_id, 1, internal_formats.at(desc.format), desc.size.x, desc.size.y);

	if (!desc.data.empty()) {
		glTextureSubImage2D(_id, 0, 0, 0, desc.size.x, desc.size.y, formats.at(desc.format), GL_UNSIGNED_BYTE, desc.data.data());
	}

	if (desc.is_render_target) {
		glCreateFramebuffers(1, &_framebuffer_id);
		glNamedFramebufferTexture(_framebuffer_id, GL_COLOR_ATTACHMENT0, _id, 0);
	}
}

texture_ogl3::~texture_ogl3() {
	glDeleteTextures(1, &_id);
	glDeleteFramebuffers(1, &_framebuffer_id);
}

void texture_ogl3::update(const span<const std::uint8_t>& pixels, const vec4i& rect) {
	glTextureSubImage2D(_id, 0, rect.x, rect.y, rect.z, rect.w, formats.at(format()), GL_UNSIGNED_BYTE, pixels.data());
}

GLuint texture_ogl3::id() const {
	return _id;
}

GLuint texture_ogl3::framebuffer_id() const {
	return _framebuffer_id;
}