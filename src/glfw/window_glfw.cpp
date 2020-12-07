#include "window_glfw.hpp"

#if RB_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <rabbit/exception.hpp>

using namespace rb;

window_glfw::window_glfw(config& config) {
    if (!glfwInit()) {
        throw exception{ "Cannot initialize GLFW" };
    }

    _window = glfwCreateWindow(config.window.size.x, config.window.size.y, config.window.title.c_str(), NULL, NULL);
    if (!_window) {
        throw exception{ "Cannot create GLFW window" };
    }

    glfwMakeContextCurrent(_window);
}

window_glfw::~window_glfw() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool window_glfw::is_open() const {
    return !glfwWindowShouldClose(_window);
}

window_handle window_glfw::native_handle() const {
#if RB_WINDOWS
    return glfwGetWin32Window(_window);
#elif RB_LINUX
    // todo: fix compilation on linux
    return 0;
#elif RB_OSX
    return glfwGetCococaWindow(_window);
#else 
    return 0;
#endif
}

void window_glfw::swap_buffers() {
    glfwSwapBuffers(_window);
}

void window_glfw::poll_events() {
    glfwPollEvents();
}

vec2i window_glfw::size() const {
    vec2i size;
    glfwGetWindowSize(_window, &size.x, &size.y);
    return size;
}

void window_glfw::maximize() {
    glfwMaximizeWindow(_window);
}

void window_glfw::set_resizable(bool resizable) const {
#if !RB_EMSCRIPTEN
    
#endif
}

bool window_glfw::is_resizable() const {
#if RB_EMSCRIPTEN
    return false;
#else
    return false;
#endif
}

bool window_glfw::is_focused() const {
#if RB_EMSCRIPTEN
    return true;
#else
    return glfwGetWindowAttrib(_window, GLFW_FOCUSED) != 0;
#endif
}

void window_glfw::set_title(const std::string& title) {
    glfwSetWindowTitle(_window, title.c_str());
}

std::string window_glfw::title() const {
    return ""; // todo
}

void window_glfw::show_cursor(bool enable) {
}

bool window_glfw::is_cursor_visible() const {
    return true;
}

GLFWwindow* window_glfw::window() const {
    return _window;
}
