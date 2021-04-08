#include "graphics_device_ogl3.hpp"
#include "texture_ogl3.hpp"
#include "buffer_ogl3.hpp"
#include "shader_ogl3.hpp"
#include "mesh_ogl3.hpp"

#include <rabbit/exception.hpp>

#include <fmt/format.h>

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
graphics_device_ogl3::graphics_device_ogl3(const config& config, window& window)
	: _window(window) {
	glewInit();

#if RB_WINDOWS
	wglewInit();
#endif

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_SCISSOR_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if RB_WINDOWS
	wglSwapIntervalEXT(config.graphics.vsync ? 1 : 0);
#endif
}

graphics_device_ogl3::~graphics_device_ogl3() {
}

std::shared_ptr<texture> graphics_device_ogl3::make_texture(const texture_desc& desc) {
	return std::make_shared<texture_ogl3>(desc);
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
	_window.swap_buffers();
}

void graphics_device_ogl3::set_blend_state(const blend_state& blend_state) {
	const auto blendEnabled = blend_state.color_source_blend != blend::one ||
		blend_state.color_destination_blend != blend::zero ||
		blend_state.alpha_source_blend != blend::one ||
		blend_state.alpha_destination_blend != blend::zero;

	if (blendEnabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	if (blendEnabled) {
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

void graphics_device_ogl3::bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) {
	if (buffer->type() != buffer_type::uniform) {
		throw make_exception("Buffer need to be uniform buffer to bind base index");
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, binding_index, std::static_pointer_cast<buffer_ogl3>(buffer)->id());
}

void graphics_device_ogl3::draw(topology topology, const std::shared_ptr<buffer>& vertex_buffer, const std::shared_ptr<shader>& shader) {
	glBindBuffer(GL_ARRAY_BUFFER, std::static_pointer_cast<buffer_ogl3>(vertex_buffer)->id());

	// todo: move to new input_layout class
	std::size_t stride = 0;

	// Iterate through vertex elements to calculate stride
	for (const auto& element : shader->vertex_desc()) {
		stride += element.format.size;
	}

	std::size_t offset = 0;
	std::size_t index = 0;

	// Iterate again to declare layout
	for (const auto& element : shader->vertex_desc()) {
		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, element.format.components, element.format.type == vertex_format_type::floating_point ? GL_FLOAT : GL_INT, element.format.normalize ? GL_TRUE : GL_FALSE, stride, (void*)offset);
		offset += element.format.size;
		index++;
	}

	glUseProgram(std::static_pointer_cast<shader_ogl3>(shader)->id());

	glDrawArrays(topologies.at(topology), 0, static_cast<GLsizei>(vertex_buffer->count()));
}
