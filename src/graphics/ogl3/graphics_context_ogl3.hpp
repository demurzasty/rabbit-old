#pragma once 

namespace rb {
    struct graphics_context_ogl3 {
        virtual ~graphics_context_ogl3() = default;

        virtual void swap_buffers() = 0;
    };
}
