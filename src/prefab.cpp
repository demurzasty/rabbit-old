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

    std::ofstream ostream{ output };
    RB_ASSERT(ostream.is_open(), "Cannot open file: {}", output);
    ostream << json;
}

std::shared_ptr<prefab> prefab::load(bstream& stream) {
    std::vector<std::uint8_t> bytes;
    bytes.resize(stream.size());
    stream.read(&bytes[0], bytes.size());

    const auto json = json::parse(bytes.begin(), bytes.end());

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
