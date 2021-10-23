# RabBit

`RabBit` is a easy to use, multiplatform engine for game develop written in **C++17**.

![Screenshot](/data/screenshot.png)

## Goals

- Provide platform implementations as many as possible.
- Producing high-quality modern code.
- Making a game engine as simple as possible.

## Requirements

To be able to use `RabBit`, users must provide a full-featured compiler that supports at least `C++17` and `CMake` version 3.16 or later.

## Supported Platforms

- ✔️ Windows (Vulkan)
- ❌ Linux (Vulkan)
- ❌ Mac OSX (Vulkan)
- ❌ Nintendo Switch
- ❌ PlayStation 5
- ❌ Xbox Series S/X

## Code Example

```cpp
#include <rabbit/rabbit.hpp>

int main(int argc, char* argv[]) {
    rb::app::setup();

    // Initialization code goes here.

    rb::app::run();
}
```

## RabBit in Action

`RabBit` is currently used in private game project [Home](https://twitter.com/HomeIndieGame).

## License

Code released under [the MIT license](https://github.com/demurzasty/rabbit/blob/master/LICENSE).
