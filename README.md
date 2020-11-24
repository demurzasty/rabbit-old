# RabBit
`RabBit` is a easy to use, multiplatform, XNA inspired framework for game programing written in **C++17**.

## Goals
* Provide platform implementations as many as possible.
* Producing high-quality modern code.
* Making a game framework as simple as possible.

## Requirements
To be able to use `RabBit`, users must provide a full-featured compiler that supports at least `C++17` and `CMake` version 3.16 or later.. 

## Backends implementations progress
### Platforms
☑️ Windows  
⭕ UWP (Universal Windows, Xbox One)  
☑️ Linux  
⭕ OSX  
⭕ Android  
⭕ iOS
### Graphics
☑️ DirectX 11  
⭕ DirectX 12  
☑️ OpenGL 3  
⭕ Vulkan  
⭕ Metal  
⭕ GNM  
### Input
☑️ Windows  
☑️ XInput

## Code Example
```cpp
#include "main.hpp"

hello_world::hello_world(rb::config& config)
    : rb::game(config) {
}

void hello_world::initialize() {
}

void hello_world::update(float elapsed_time) {
    if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
        keyboard()->is_key_pressed(rb::keycode::escape)) {
        exit();
    }
}

void hello_world::draw() {
    graphics_device()->clear(rb::color::cornflower_blue());
}

int main(int argc, char* argv[]) {
    rb::config config;
    config.window_title = "Hello World";
    config.window_size = { 1280, 720 };
    hello_world{ config }.run();
}
```

## RabBit in Action
`RabBit` is currently used in private game project [Home](https://twitter.com/HomeIndieGame).

## License
Code released under [the MIT license](https://github.com/demurzasty/rabbit/blob/master/LICENSE). 
