#include "graphics_device_ogl3.hpp"
#include "standard_shaders_ogl3.hpp"
#include "texture_ogl3.hpp"
#include "buffer_ogl3.hpp"
#include "shader_ogl3.hpp"

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
	glGenBuffers(1, &_vbo);
	glGenBuffers(1, &_ibo);

	glBindVertexArray(_vao);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * 4, NULL, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texcoord));

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void*)offsetof(vertex, color));

	_solid_program = compile_program(standard_shaders_ogl3::vertex_shader(), standard_shaders_ogl3::solid_pixel_shader());
	_texture_program = compile_program(standard_shaders_ogl3::vertex_shader(), standard_shaders_ogl3::texture_pixel_shader());

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

void graphics_device_ogl3::set_view_matrix(const mat4f& view) {
	_view = view;
}

void graphics_device_ogl3::set_projection_matrix(const mat4f& projection) {
	_projection = projection;
}

void graphics_device_ogl3::set_world_matrix(const mat4f& world) {
	_world = world;
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
		throw make_exception("Buffer need to be uniform buffer to bind base");
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, binding_index, std::static_pointer_cast<buffer_ogl3>(buffer)->id());
}

void graphics_device_ogl3::draw(topology topology, const std::shared_ptr<buffer>& vertex_buffer, const std::shared_ptr<shader>& shader) {
	glBindBuffer(GL_ARRAY_BUFFER, std::static_pointer_cast<buffer_ogl3>(vertex_buffer)->id());

	// todo: use vertex_desc to define these
	struct mesh_vertex {
		vec3f position;
		vec2f texcoord;
		vec3f normal;
	};

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(mesh_vertex), (void*)offsetof(mesh_vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(mesh_vertex), (void*)offsetof(mesh_vertex, texcoord));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(mesh_vertex), (void*)offsetof(mesh_vertex, normal));

	glUseProgram(std::static_pointer_cast<shader_ogl3>(shader)->id());

	glDrawArrays(topologies.at(topology), 0, static_cast<GLsizei>(vertex_buffer->count()));
}

GLuint graphics_device_ogl3::compile_program(const char* vertex_shader_code, const char* fragment_shader_code) const {
	const auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	const char* vertex_shader_codes[] = { vertex_shader_code };
	const char* fragment_shader_codes[] = { fragment_shader_code };

	glShaderSource(vertex_shader, sizeof(vertex_shader_codes) / sizeof(*vertex_shader_codes), vertex_shader_codes, nullptr);
	glShaderSource(fragment_shader, sizeof(fragment_shader_codes) / sizeof(*fragment_shader_codes), fragment_shader_codes, nullptr);

	glCompileShader(vertex_shader);
	glCompileShader(fragment_shader);

	GLint vertex_result, fragment_result;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_result);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_result);

	if (!vertex_result || !fragment_result) {
		GLint vertex_info_length, fragment_info_length;
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &vertex_info_length);
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &fragment_info_length);

		std::string vertex_message(vertex_info_length, '\000');
		std::string fragment_message(fragment_info_length, '\000');

		if (vertex_info_length > 0) {
			glGetShaderInfoLog(vertex_shader, vertex_info_length, nullptr, &vertex_message[0]);
		}

		if (fragment_info_length > 0) {
			glGetShaderInfoLog(fragment_shader, fragment_info_length, nullptr, &fragment_message[0]);
		}

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		throw make_exception("Cannot compile program: {}\n{}", vertex_message, fragment_message);
	}

	const auto program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);

	GLint program_result;
	glGetProgramiv(program, GL_LINK_STATUS, &program_result);
	if (!program_result) {
		GLint program_info_length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &program_info_length);

		std::string program_message(program_info_length, '\000');
		glGetProgramInfoLog(program, program_info_length, NULL, &program_message[0]);

		glDetachShader(program, vertex_shader);
		glDetachShader(program, fragment_shader);

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		glDeleteProgram(program);

		throw make_exception("Cannot link program: {}", program_message);
	}

	return program;
}

void graphics_device_ogl3::update_vertex_buffer(const span<const vertex>& vertices) {
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);

	GLint current_size;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_size);

	if (vertices.size_bytes() <= current_size) {
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size_bytes(), vertices.data());
	} else {
		glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), GL_DYNAMIC_DRAW);
	}
}

void graphics_device_ogl3::update_index_buffer(const span<const std::uint32_t>& indices) {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

	GLint current_size;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_size);

	if (indices.size_bytes() <= current_size) {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size_bytes(), indices.data());
	} else {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), GL_DYNAMIC_DRAW);
	}
}
