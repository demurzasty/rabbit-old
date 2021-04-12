#pragma once 

#include "../graphics_context_ogl3.hpp"

#include <rabbit/platform/window.hpp>

struct HWND__;
struct HDC__;
struct HGLRC__;

namespace rb {
    class graphics_contrext_ogl3_win32 : public graphics_context_ogl3 {
    public:
        graphics_contrext_ogl3_win32(window& window);

        ~graphics_contrext_ogl3_win32();
        
        void swap_buffers() override;

    private:
        HWND__* _hwnd;
        HDC__* _hdc;
        HGLRC__* _hglrc;
    };
}