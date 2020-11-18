#pragma once 

#include "loader.hpp"
#include "graphics_device.hpp"

namespace rb {
    class texture_loader : public loader {
    public:
        texture_loader(std::shared_ptr<graphics_device> graphics_device);

        virtual std::shared_ptr<void> load(const std::string& filename) override;

    private:
        std::shared_ptr<graphics_device> _graphics_device;
    };
}