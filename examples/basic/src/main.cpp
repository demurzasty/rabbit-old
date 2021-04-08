#include <rabbit/rabbit.hpp>

#include <filesystem>
#include <iostream>

class test_system : public rb::system {
public:
    test_system(rb::asset_manager& asset_manager)
        : _asset_manager(asset_manager) {
    }

    void initialize(rb::registry& registry) override {
        auto mesh = _asset_manager.load<rb::mesh>("rabbit_base.obj");

        auto sphere = registry.create();
        registry.emplace<rb::geometry>(sphere, mesh);
        registry.emplace<rb::transform>(sphere);

        mesh = _asset_manager.load<rb::mesh>("rabbit_sphere.obj");
        sphere = registry.create();
        registry.emplace<rb::geometry>(sphere, mesh);
        registry.emplace<rb::transform>(sphere);
    }

private:
    rb::asset_manager& _asset_manager;
};

int main(int argc, char* argv[]) {
    std::filesystem::current_path(DATA_DIRECTORY);

    auto game = rb::make_default_builder()
        .system<test_system>()
        .build();
    
    return game.run();
}
