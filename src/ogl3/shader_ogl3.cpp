#include "shader_ogl3.hpp"

#include <rabbit/exception.hpp>

#include <cstring>

using namespace rb;

shader_ogl3::shader_ogl3(const shader_desc& desc) {
    // Retrieve code from bytecode
    std::string vertex_shader_code(desc.vertex_bytecode.size(), '\000');
    std::string fragment_shader_code(desc.fragment_bytecode.size(), '\000');

    // Copy from bytecode to strings
    std::memcpy(&vertex_shader_code[0], desc.vertex_bytecode.data(), desc.vertex_bytecode.size());
    std::memcpy(&fragment_shader_code[0], desc.fragment_bytecode.data(), desc.fragment_bytecode.size());

    // Create OpenGL shader objects
    const auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    // Save as array of string pointers
	const char* vertex_shader_codes[] = { vertex_shader_code.c_str() };
	const char* fragment_shader_codes[] = { fragment_shader_code.c_str() };

    // Send code to driver
	glShaderSource(vertex_shader, sizeof(vertex_shader_codes) / sizeof(*vertex_shader_codes), vertex_shader_codes, nullptr);
	glShaderSource(fragment_shader, sizeof(fragment_shader_codes) / sizeof(*fragment_shader_codes), fragment_shader_codes, nullptr);

    // Try to compile shaders
	glCompileShader(vertex_shader);
	glCompileShader(fragment_shader);

    // Check if some errors occured
	GLint vertex_result, fragment_result;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_result);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_result);

    // Handle errors
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

	_program = glCreateProgram();

	glAttachShader(_program, vertex_shader);
	glAttachShader(_program, fragment_shader);

	glLinkProgram(_program);

	GLint program_result;
	glGetProgramiv(_program, GL_LINK_STATUS, &program_result);
	if (!program_result) {
		GLint program_info_length;
		glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &program_info_length);

		std::string program_message(program_info_length, '\000');
		glGetProgramInfoLog(_program, program_info_length, NULL, &program_message[0]);

		glDetachShader(_program, vertex_shader);
		glDetachShader(_program, fragment_shader);

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		glDeleteProgram(_program);

		throw make_exception("Cannot link program: {}", program_message);
	}
}

shader_ogl3::~shader_ogl3() {
	glDeleteProgram(_program);
}

GLuint shader_ogl3::id() const {
    return _program;
}