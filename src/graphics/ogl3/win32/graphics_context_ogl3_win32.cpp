#include "graphics_context_ogl3_win32.hpp"

#include <Windows.h>

using namespace rb;

graphics_contrext_ogl3_win32::graphics_contrext_ogl3_win32(window& window)
    : _hwnd(window.native_handle()) {
    PIXELFORMATDESCRIPTOR pixel_format_desc = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    _hdc = GetDC(_hwnd);

    SetPixelFormat(_hdc, ChoosePixelFormat(_hdc, &pixel_format_desc), &pixel_format_desc);

    _hglrc = wglCreateContext(_hdc);

    wglMakeCurrent(_hdc, _hglrc);
}

graphics_contrext_ogl3_win32::~graphics_contrext_ogl3_win32() {
    wglDeleteContext(_hglrc);
    ReleaseDC(_hwnd, _hdc);
}

void graphics_contrext_ogl3_win32::swap_buffers() {
    SwapBuffers(_hdc);
}
