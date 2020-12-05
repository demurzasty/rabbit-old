# RabBit
`RabBit` is a easy to use, multiplatform, XNA inspired framework for game programing written in **C++17**.

## Goals
* Provide platform implementations as many as possible.
* Producing high-quality modern code.
* Making a game framework as simple as possible.

## Requirements
To be able to use `RabBit`, users must provide a full-featured compiler that supports at least `C++17` and `CMake` version 3.16 or later.. 

## Supported Platforms
* ✔️ Windows (OpenGL & DirectX)
* ✔️ Linux (OpenGL)
* ⚠️ Mac OSX (OpenGL)
* ❌ Nintendo Switch
* ❌ PlayStation 5
* ❌ Xbox Series S/X

## Code Example
```cpp
#include <rabbit/rabbit.hpp>

using namespace rb;

class example_game : public rb::game {
public:
    example_game(rb::config& config)
        : rb::game(config) {
    }

    void initialize() {
    }

    void update(float elapsed_time) {
        // Exit game if back button on gamepad or escape on keyboard was pressed.
        if (gamepad()->is_button_pressed(rb::gamepad_player::first, rb::gamepad_button::back) ||
            keyboard()->is_key_pressed(rb::keycode::escape)) {
            exit();
        }
    }

    void draw() {
        // Clear window with color.
        graphics_device()->clear(rb::color::cornflower_blue());
    }
};

int main(int argc, char* argv[]) {
    rb::config config;
    config.window.title = "Hello World";
    config.window.size = { 1280, 720 };
    example_game{ config }.run();
}
```

## RabBit in Action
`RabBit` is currently used in private game project [Home](https://twitter.com/HomeIndieGame).

## License
Code released under [the MIT license](https://github.com/demurzasty/rabbit/blob/master/LICENSE). 
