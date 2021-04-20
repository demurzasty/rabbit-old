#include <rabbit/rabbit.hpp>
#include <rabbit/buffer.hpp>

int main(int argc, char* argv[]) {
    auto app = rb::make_builder()
        .build();

    app.run();
}
