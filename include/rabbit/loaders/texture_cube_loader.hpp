#pragma once 

#include "../asset/loader.hpp"
#include "../graphics/graphics_device.hpp"

namespace rb {
    class texture_cube_loader : public loader {
    public:
        texture_cube_loader(graphics_device& graphics_device);

        virtual std::shared_ptr<void> load(const std::string& filename, const json& metadata) override;

    private:
        graphics_device& _graphics_device;
    };
}
