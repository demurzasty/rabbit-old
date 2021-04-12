#include "graphics_context_ogl3_win32.hpp"

#include <rabbit/core/exception.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <GL/glew.h>
#include <GL/wglew.h>

using namespace rb;

graphics_context_ogl3_win32::graphics_context_ogl3_win32(window& window)
    : _hwnd(window.native_handle()) {
    PIXELFORMATDESCRIPTOR pixel_format_desc = {
        sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format Descriptor
        1,                              // Version Number
        PFD_DRAW_TO_WINDOW |            // Format Must Support Window
        PFD_SUPPORT_OPENGL |            // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,               // Format Must Support Doublebuffering
        PFD_TYPE_RGBA,                  // Request An RGBA Format
        24,                             // Select Our Color Depth
        8, 0, 8, 0, 8, 0,               // Color Bits Ignored
        0, 0,                           // Shift Bit Ignored
        0,                              // No Accumulation Buffer
        0, 0, 0, 0,                     // Accumulation Bits Ignored
        24,                             // 16Bit Z-Buffer (Depth Buffer)
        0,                              // No Stencil Buffer
        0,                              // No Auxiliary Buffer
        PFD_MAIN_PLANE,                 // Main Drawing Layer
        0,                              // Reserved
        0, 0, 0                         // Layer Masks Ignored
    };

    _hdc = GetDC(_hwnd);
    if (!_hdc) {
        throw make_exception("Cannot retrieve device context");
    }

    auto pixel_format = ChoosePixelFormat(_hdc, &pixel_format_desc);
    if (pixel_format == 0) {
        throw make_exception("Cannot choose pixel format");
    }

    if (!SetPixelFormat(_hdc, pixel_format, &pixel_format_desc)) {
        throw make_exception("Cannot set pixel format");
    }

    if (!SetPixelFormat(_hdc, pixel_format, &pixel_format_desc)) {
        throw make_exception("Cannot set pixel format");
    }

    auto temp_hglrc = wglCreateContext(_hdc);

    wglMakeCurrent(_hdc, temp_hglrc);

    if (const auto error = glewInit(); error != GLEW_OK) {
        throw make_exception("Cannot initialize GLEW: {}", glewGetErrorString(error));
    }

    if (const auto error = wglewInit(); error != GLEW_OK) {
        throw make_exception("Cannot initialize WGLEW: {}", glewGetErrorString(error));
    }

    int pixel_format_attribs[] = { 
        WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
        WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,          
        WGL_COLOR_BITS_ARB,         24,
        WGL_RED_BITS_ARB,           8,
        WGL_GREEN_BITS_ARB,         8,
        WGL_BLUE_BITS_ARB,          8,
        WGL_ALPHA_BITS_ARB,         0,
        WGL_DEPTH_BITS_ARB,         24,
        WGL_STENCIL_BITS_ARB,       0,
        // WGL_SAMPLE_BUFFERS_ARB,  GL_TRUE,
        // WGL_SAMPLES_ARB,         8,
        WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
        0, 0 
    };

    UINT formats = 0;
    if (!wglChoosePixelFormatARB(_hdc, pixel_format_attribs, nullptr, 1, &pixel_format, &formats) ||
        formats < 1) {
        throw make_exception("Cannot choose pixel format: {}", GetLastError());
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(temp_hglrc);

    if (!SetPixelFormat(_hdc, pixel_format, &pixel_format_desc)) {
        throw make_exception("Cannot set pixel format: {}", GetLastError());
    }

    if (wglewIsSupported("WGL_ARB_create_context")) {
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_FLAGS_ARB, 0,
            0
        };

        _hglrc = wglCreateContextAttribsARB(_hdc, nullptr, attribs);
    } else {
        _hglrc = wglCreateContext(_hdc);
    }

    wglMakeCurrent(_hdc, _hglrc);
}

graphics_context_ogl3_win32::~graphics_context_ogl3_win32() {
    wglDeleteContext(_hglrc);
    ReleaseDC(_hwnd, _hdc);
}

void graphics_context_ogl3_win32::set_vsync(bool vsync) {
    wglSwapIntervalEXT(vsync ? 1 : 0);
}

void graphics_context_ogl3_win32::swap_buffers() {
    SwapBuffers(_hdc);
}
