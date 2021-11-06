#include <rabbit/prefab.hpp>
#include <rabbit/config.hpp>
#include <rabbit/app.hpp>
#include <rabbit/transform.hpp>

#include <fstream>

using namespace rb;

void prefab::import(ibstream& input, obstream& output, const json& metadata) {
    json json;
    input.read(json);

    const auto dump = json::to_cbor(json);

    output.write(prefab::magic_number);
    output.write<std::uint32_t>(dump.size());
    output.write(&dump[0], dump.size());
}

std::shared_ptr<prefab> prefab::load(ibstream& stream) {
    std::vector<std::uint8_t> bytes;

    std::uint32_t size;
    stream.read(size);
    bytes.resize(size);
    stream.read(&bytes[0], bytes.size());

    return std::shared_ptr<prefab>(new prefab(json::from_cbor(bytes)));
}

void prefab::apply(registry& registry) {
    if (_json.contains("entities")) {
        _apply_entities(registry, _json["entities"], null);
    }
}

prefab::prefab(json json)
    : _json(json) {
}

void prefab::_apply_entities(registry& registry, const json& jentities, entity parent) {
    for (auto jentity : jentities) {
        auto entity = registry.create();
        if (registry.valid(parent)) {
            registry.get_or_emplace<transform>(entity).parent = parent;
        }

        for (auto& item : jentity.items()) {
            if (item.key() == "children") {
                _apply_entities(registry, item.value(), entity);
            } else {
                json_read_visitor visitor{ item.value() };

                const auto deserializer = app::get_deserializer(item.key());
                deserializer(registry, entity, visitor);
            }
        }
    }
}
