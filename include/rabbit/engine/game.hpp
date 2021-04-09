#pragma once 

#include "builder.hpp"
#include "../core/config.hpp"
#include "../entity/entity.hpp"
#include "../platform/window.hpp"
#include "../graphics/graphics_device.hpp"
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"
#include "../input/gamepad.hpp"
#include "../asset/asset_manager.hpp"

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