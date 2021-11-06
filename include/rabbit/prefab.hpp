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

        static void import(ibstream& input, obstream& output, const json& metadata);

        static std::shared_ptr<prefab> load(ibstream& stream);

        void apply(registry& registry);

    private:
        prefab(json json);

        void _apply_entities(registry& registry, const json& jentities, entity parent);

    private:
        json _json;
    };
}
