#pragma once 

#include "builder.hpp"
#include "config.hpp"
#include "entity.hpp"
#include "window.hpp"
#include "graphics_device.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"
#include "gamepad.hpp"
#include "asset_manager.hpp"
#include "state_manager.hpp"

#include <map>
#include <memory>

namespace rb {
    class game {
    public:
        game(const builder& builder);

        game(const game&) = delete;
        game(game&&) = default;

        game& operator=(const game&) = delete;
        game& operator=(game&&) = default;

        int run();

    private:
        container _container;
        registry _registry;
        std::vector<std::shared_ptr<rb::system>> _systems;
    };
}