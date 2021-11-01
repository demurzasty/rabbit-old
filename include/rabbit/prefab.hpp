#pragma once 

#include "json.hpp"
#include "bstream.hpp"
#include "entity.hpp"

#include <string>
#include <memory>
#include <functional>

namespace rb {
    class prefab {
    public:
        static void import(const std::string& input, const std::string& output, const json& metadata);

        static std::shared_ptr<prefab> load(bstream& stream);

        prefab(std::function<void(registry&)> applier);

        void apply(registry& registry);

    private:
        std::function<void(registry&)> _applier;
    };
}
