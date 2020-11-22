#pragma once 

#include "config.hpp"
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
        game(config& config);

        void run();

        void exit();

        const config& config() const;

        std::shared_ptr<window> window() const;

        std::shared_ptr<graphics_device> graphics_device() const;

        std::shared_ptr<keyboard> keyboard() const;

        std::shared_ptr<mouse> mouse() const;

        std::shared_ptr<gamepad> gamepad() const;

        std::shared_ptr<asset_manager> asset_manager() const;

        std::shared_ptr<state_manager> state_manager() const;

    protected:
        virtual void initialize();

        virtual void release();

        virtual void update(float elapsed_time);

        virtual void fixed_update(float fixed_time);

        virtual void draw();

    private:
        rb::config& _config;
        std::shared_ptr<rb::window> _window;
        std::shared_ptr<rb::graphics_device> _graphics_device;
        std::shared_ptr<rb::keyboard> _keyboard;
        std::shared_ptr<rb::mouse> _mouse;
        std::shared_ptr<rb::gamepad> _gamepad;
        std::shared_ptr<rb::asset_manager> _asset_manager;
        std::shared_ptr<rb::state_manager> _state_manager;
        bool _running = true;
    };
}