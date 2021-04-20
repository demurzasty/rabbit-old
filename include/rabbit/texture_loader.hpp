#pragma once 

#include "loader.hpp"
#include "graphics_device.hpp"

namespace rb {
    class texture_loader : public loader {
    public:
        texture_loader(graphics_device& graphics_device);

        std::shared_ptr<void> load(const std::string& filename, const json& json) override;
    
    private:
        graphics_device& _graphics_device;
    };
}