#include "window_sdl2.hpp"

#include <rabbit/exception.hpp>

#include <fmt/format.h>

using namespace rb;

window_sdl2::window_sdl2(config& config) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        throw exception{ fmt::format("Cannot initialize SDL2: {}", SDL_GetError()) };
    }

    Uint32 flags = SDL_WINDOW_SHOWN;

    if (config.window_resizable) { 
        flags |= SDL_WINDOW_RESIZABLE;
    }

    if (config.window_borderless) {
        flags |= SDL_WINDOW_BORDERLESS;
    }

    _window = SDL_CreateWindow(config.window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.window_size.x, config.window_size.y, flags);
    if (!_window) {
        throw exception{ fmt::format("Cannot create window: {}", SDL_GetError()) };
    }

    SDL_GL_CreateContext(_window);
}

window_sdl2::~window_sdl2() {
    SDL_DestroyWindow(_window);
}

bool window_sdl2::is_open() const {
    return _open;
}

window_handle window_sdl2::native_handle() const {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(_window, &info);

#if RB_WINDOWS
    return info.info.win.window;
#elif RB_LINUX
    return info.info.x11.window;
#elif RB_OSX
    return info.info.cocoa.window;
#elif RB_IOS
    return info.info.uikit.window;
#else 
    return 0;
#endif
}

void window_sdl2::poll_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            _open = false;
        }
    }
}

vec2i window_sdl2::size() const {
    vec2i size;
    SDL_GetWindowSize(_window, &size.x, &size.y);
    return size;
}

void window_sdl2::maximize() {
    SDL_MaximizeWindow(_window);
}

void window_sdl2::set_resizable(bool resizable) const {
    SDL_SetWindowResizable(_window, static_cast<SDL_bool>(resizable));
}

bool window_sdl2::is_resizable() const {
    return SDL_GetWindowFlags(_window) & SDL_WINDOW_RESIZABLE;
}

bool window_sdl2::is_focused() const {
    return SDL_GetWindowFlags(_window) & SDL_WINDOW_INPUT_FOCUS;
}

void window_sdl2::set_title(const std::string& title) {
    SDL_SetWindowTitle(_window, title.c_str());
}

std::string window_sdl2::title() const {
    return SDL_GetWindowTitle(_window);
}
