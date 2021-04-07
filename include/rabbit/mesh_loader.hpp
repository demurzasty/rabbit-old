#pragma once 

#include "loader.hpp"
#include "graphics_device.hpp"

namespace rb {
    class mesh_loader : public loader {
    public:
        mesh_loader(graphics_device& graphics_device);

        std::shared_ptr<void> load(const std::string& filename, const json& metadata) override;
        
    private:
        graphics_device& _graphics_device;
    }
}
