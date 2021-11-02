#include <rabbit/prefab.hpp>
#include <rabbit/config.hpp>
#include <rabbit/app.hpp>

#include <fstream>

using namespace rb;

void prefab::import(const std::string& input, const std::string& output, const json& metadata) {
    std::ifstream istream{ input };
    RB_ASSERT(istream.is_open(), "Cannot open file: {}", input);

    json json;
    istream >> json;
    istream.close();

    const auto dump = json::to_cbor(json);

    bstream ostream{ output, bstream_mode::write };
    ostream.write(prefab::magic_number);
    ostream.write<std::uint32_t>(dump.size());
    ostream.write(&dump[0], dump.size());
}

std::shared_ptr<prefab> prefab::load(bstream& stream) {
    std::vector<std::uint8_t> bytes;

    std::uint32_t size;
    stream.read(size);
    bytes.resize(size);
    stream.read(&bytes[0], bytes.size());

    const auto json = json::from_cbor(bytes);

    const auto applier = [json](registry& registry) {
        for (auto jentity : json["entities"]) {
            auto entity = registry.create();
            for (auto& item : jentity.items()) {
                json_read_visitor visitor{ item.value() };

                const auto deserializer = app::get_deserializer(item.key());
                deserializer(registry, entity, visitor);
            }
        }
    };

    return std::make_shared<prefab>(applier);
}

prefab::prefab(std::function<void(registry&)> applier)
    : _applier(applier) {
}

void prefab::apply(registry& registry) {
    _applier(registry);
}
