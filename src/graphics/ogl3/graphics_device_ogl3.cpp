#include "graphics_device_ogl3.hpp"
#include "texture_ogl3.hpp"
#include "texture_cube_ogl3.hpp"
#include "buffer_ogl3.hpp"
#include "shader_ogl3.hpp"
#include "mesh_ogl3.hpp"

#if RB_WINDOWS
#include "win32/graphics_context_ogl3_win32.hpp"
namespace rb { using graphics_context_ogl3_impl = graphics_context_ogl3_win32; }
#endif

#include <rabbit/core/exception.hpp>

#include <map>

using namespace rb;

static std::map<topology, GLenum> topologies = {
	{ topology::lines, GL_LINES },
	{ topology::line_strip, GL_LINE_STRIP },
	{ topology::triangles, GL_TRIANGLES },
	{ topology::triangle_strip, GL_TRIANGLE_STRIP }
};

static std::map<blend_function, GLenum> blend_equation_modes = {
	{ blend_function::add, GL_FUNC_ADD },
	{ blend_function::subtract, GL_FUNC_SUBTRACT },
	{ blend_function::reverse_subtract, GL_FUNC_REVERSE_SUBTRACT },
	{ blend_function::min, GL_MIN },
	{ blend_function::max, GL_MAX },
};

static std::map<blend, GLenum> blend_factors = {
	{ blend::blend_factor, GL_CONSTANT_COLOR },
	{ blend::destination_alpha, GL_DST_ALPHA },
	{ blend::destination_color, GL_DST_COLOR },
	{ blend::inverse_blend_factor, GL_ONE_MINUS_CONSTANT_COLOR },
	{ blend::inverse_destination_alpha, GL_ONE_MINUS_DST_ALPHA },
	{ blend::inverse_destination_color, GL_ONE_MINUS_DST_COLOR },
	{ blend::inverse_source_alpha, GL_ONE_MINUS_SRC_ALPHA },
	{ blend::inverse_source_color, GL_ONE_MINUS_SRC_COLOR },
	{ blend::one, GL_ONE },
	{ blend::zero, GL_ZERO },
	{ blend::source_alpha, GL_SRC_ALPHA },
	{ blend::source_alpha_saturation, GL_SRC_ALPHA_SATURATE },
	{ blend::source_color, GL_SRC_COLOR }
};

static std::map<texture_cube_face, GLint> texture_cube_faces = {
	{ texture_cube_face::positive_x, GL_TEXTURE_CUBE_MAP_POSITIVE_X },
	{ texture_cube_face::negative_x, GL_TEXTURE_CUBE_MAP_NEGATIVE_X },
	{ texture_cube_face::positive_y, GL_TEXTURE_CUBE_MAP_POSITIVE_Y },
	{ texture_cube_face::negative_y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y },
	{ texture_cube_face::positive_z, GL_TEXTURE_CUBE_MAP_POSITIVE_Z },
	{ texture_cube_face::negative_z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z },
};

graphics_device_ogl3::graphics_device_ogl3(const config& config, window& window)
	: _window(window) {
	_context = std::make_shared<graphics_context_ogl3_impl>(window);

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param) {
		fprintf(stderr, "%s\n", message);
	}, nullptr);

	_context->set_vsync(config.graphics.vsync);
}

graphics_device_ogl3::~graphics_device_ogl3() {
}

std::shared_ptr<texture> graphics_device_ogl3::make_texture(const texture_desc& desc) {
	return std::make_shared<texture_ogl3>(desc);
}

std::shared_ptr<texture_cube> graphics_device_ogl3::make_texture(const texture_cube_desc& texture_desc) {
	return std::make_shared<texture_cube_ogl3>(texture_desc);
}

std::shared_ptr<buffer> graphics_device_ogl3::make_buffer(const buffer_desc& buffer_desc) {
	return std::make_shared<buffer_ogl3>(buffer_desc);
}

std::shared_ptr<shader> graphics_device_ogl3::make_shader(const shader_desc& shader_desc) {
	return std::make_shared<shader_ogl3>(shader_desc);
}

std::shared_ptr<mesh> graphics_device_ogl3::make_mesh(const mesh_desc& mesh_desc) {
	return std::make_shared<mesh_ogl3>(mesh_desc);
}

void graphics_device_ogl3::clear(const color& color) {
	const auto rgba = color.to_vec4<float>();

	glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graphics_device_ogl3::present() {
	_context->swap_buffers();
}

void graphics_device_ogl3::set_blend_state(const blend_state& blend_state) {
	const auto blend_enabled = blend_state.color_source_blend != blend::one ||
		blend_state.color_destination_blend != blend::zero ||
		blend_state.alpha_source_blend != blend::one ||
		blend_state.alpha_destination_blend != blend::zero;

	if (blend_enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	if (blend_enabled) {
		glBlendEquationSeparate(blend_equation_modes.at(blend_state.color_blend_function),
			blend_equation_modes.at(blend_state.alpha_blend_function));

		glBlendFuncSeparate(blend_factors.at(blend_state.color_source_blend),
			blend_factors.at(blend_state.color_destination_blend),
			blend_factors.at(blend_state.alpha_source_blend),
			blend_factors.at(blend_state.alpha_destination_blend));
	}
}

void graphics_device_ogl3::set_depth_test(bool depth_test) {
	if (depth_test) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}

void graphics_device_ogl3::set_backbuffer_size(const vec2i& size) {
	glViewport(0, 0, size.x, size.y);
	set_clip_rect({ 0, 0, size.x, size.y });
}

vec2i graphics_device_ogl3::backbuffer_size() const {
	// todo: should we get it from cache?
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	return { viewport[2], viewport[3] };
}

void graphics_device_ogl3::set_clip_rect(const vec4i& rect) {
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glScissor(static_cast<GLint>(rect.x),
		static_cast<GLint>(viewport[3]) - static_cast<GLint>(rect.y) - static_cast<GLint>(rect.w),
		static_cast<GLint>(rect.z),
		static_cast<GLsizei>(rect.w));
}

void graphics_device_ogl3::set_render_target(const std::shared_ptr<texture>& texture) {
	if (texture) {
		glBindFramebuffer(GL_FRAMEBUFFER, std::static_pointer_cast<texture_ogl3>(texture)->framebuffer_id());
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void graphics_device_ogl3::set_render_target(const std::shared_ptr<texture_cube>& render_target, texture_cube_face face, int level) {
	if (render_target) {
		const auto id = std::static_pointer_cast<texture_cube_ogl3>(render_target)->framebuffer_id();
		glBindFramebuffer(GL_FRAMEBUFFER, id);
		glNamedFramebufferTextureLayer(id, GL_COLOR_ATTACHMENT0, std::static_pointer_cast<texture_cube_ogl3>(render_target)->id(),
			level, (int)face);
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
} 

void graphics_device_ogl3::bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) {
	if (buffer->type() != buffer_type::uniform) {
		throw make_exception("Buffer need to be uniform buffer to bind base index");
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, binding_index, std::static_pointer_cast<buffer_ogl3>(buffer)->id());
}

void graphics_device_ogl3::bind_texture(const std::shared_ptr<texture>& texture, std::size_t binding_index) {
	glBindTextureUnit(binding_index, std::static_pointer_cast<texture_ogl3>(texture)->id());
}
        
void graphics_device_ogl3::bind_texture(const std::shared_ptr<texture_cube>& texture, std::size_t binding_index) {
	glBindTextureUnit(binding_index, std::static_pointer_cast<texture_cube_ogl3>(texture)->id());
}

void graphics_device_ogl3::draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader) {
	auto native_mesh = std::static_pointer_cast<mesh_ogl3>(mesh);

	glBindVertexArray(native_mesh->id());
	glUseProgram(std::static_pointer_cast<shader_ogl3>(shader)->id());
	
	if (mesh->index_buffer()) {
		// todo: do not use hard coded GL_UNSIGNED_INT
		glDrawElements(topologies.at(mesh->topology()), mesh->index_buffer()->count(), GL_UNSIGNED_INT, nullptr);
	} else {
		glDrawArrays(topologies.at(mesh->topology()), 0, static_cast<GLsizei>(mesh->vertex_buffer()->count()));
	}
}
