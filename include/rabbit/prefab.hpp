#pragma once 

#include "json.hpp"
#include "bstream.hpp"
#include "entity.hpp"
#include "fnv1a.hpp"

#include <string>
#include <memory>
#include <functional>

namespace rb {
    class prefab {
    public:
        static constexpr auto magic_number{ fnv1a("prefab") };

        static void import(const std::string& input, bstream& output, const json& metadata);

        static std::shared_ptr<prefab> load(bstream& stream);

        prefab(std::function<void(registry&)> applier);

        void apply(registry& registry);

    private:
        std::function<void(registry&)> _applier;
    };
}
