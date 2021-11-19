#pragma once 

#include <rabbit/engine/core/json.hpp>
#include <rabbit/engine/core/bstream.hpp>
#include <rabbit/engine/core/uuid.hpp>
#include <rabbit/engine/core/fnv1a.hpp>
#include <rabbit/engine/core/entity.hpp>

#include <string>
#include <memory>
#include <functional>

namespace rb {
    class prefab {
    public:
        static constexpr auto magic_number{ fnv1a("prefab") };

        static void import(ibstream& input, obstream& output, const json& metadata);

        static std::shared_ptr<prefab> load(ibstream& stream);

        void apply(registry& registry, entity parent);

    private:
        prefab(json json);

        void _apply_entities(registry& registry, const json& jentities, entity parent);

    private:
        json _json;
    };
}
