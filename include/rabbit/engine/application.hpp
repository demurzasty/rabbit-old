#pragma once

#include "../core/di.hpp"
#include "system.hpp"
#include "../math/vec2.hpp"

#include <list>
#include <memory>
#include <string>

namespace rb {
    class builder;

    struct application_config {
        struct {
            std::string title{ "RabBit" };
            vec2i size{ 1280, 720 };
        } window;
    };

    struct application_state {
        bool running = true;
    };

    class application {
    public:
        application(const builder& builder);

        application(const application&) = delete;
        application& operator=(const application&) = delete;

        application(application&&) = default;
        application& operator=(application&&) = default;

        int run();

    private:
        injector _injector;
        std::list<std::shared_ptr<rb::system>> _systems;
        registry _registry;
    };
}
