#include <rabbit/rabbit.hpp>

int main(int argc, char* argv[]) {
    auto game = rb::make_default_builder()
        .build();
    
    return game.run();
}
