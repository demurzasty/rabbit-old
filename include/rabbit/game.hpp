#pragma once 

#include "config.hpp"
#include "window.hpp"
#include "graphics_device.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"
#include "gamepad.hpp"
#include "asset_manager.hpp"
#include "state_manager.hpp"
#include "injector.hpp"

#include <map>
#include <memory>

namespace rb {
    class game {
    public:
        game(std::shared_ptr<rb::config> config);

        void run();

        void exit();

        std::shared_ptr<rb::config> config() const;

        std::shared_ptr<rb::window> window() const;

        std::shared_ptr<rb::graphics_device> graphics_device() const;

        std::shared_ptr<rb::keyboard> keyboard() const;

        std::shared_ptr<rb::mouse> mouse() const;

        std::shared_ptr<rb::gamepad> gamepad() const;

        std::shared_ptr<rb::asset_manager> asset_manager() const;

        std::shared_ptr<rb::state_manager> state_manager() const;

    protected:
        virtual void initialize();

        virtual void release();

        virtual void update(float elapsed_time);

        virtual void fixed_update(float fixed_time);

        virtual void draw();

    private:
        mutable dependency_container _container;
        bool _running = true;
    };
}