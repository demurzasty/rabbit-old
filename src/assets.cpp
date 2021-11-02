#include <rabbit/assets.hpp>

#include <fstream>

using namespace rb;

std::unordered_map<std::type_index, assets::loader> assets::_loaders;
std::unordered_map<uuid, std::weak_ptr<void>, uuid::hasher> assets::_assets;

void assets::init() {
}

void assets::release() {
	_assets.clear();
	_loaders.clear();
}

std::shared_ptr<void> assets::_load(std::type_index type_index, const uuid& uuid, fnv1a_result_t magic_number) {
    auto& asset = _assets[uuid];
    if (!asset.expired()) {
        return asset.lock();
    }

    fnv1a_result_t asset_magic_number;

    bstream stream{ "package/" + uuid.to_string(), bstream_mode::read };
    stream.read(asset_magic_number);

    RB_ASSERT(asset_magic_number == magic_number, "Asset type is not compatible.");

    auto& loader = _loaders.at(type_index);
    auto loaded_asset = loader(stream);
    asset = loaded_asset;
    return loaded_asset;
}

uuid assets::get_uuid(const std::shared_ptr<void>& asset) {
    for (const auto& [uuid, loaded_asset] : _assets) {
        if (!loaded_asset.expired() && loaded_asset.lock() == asset) {
            return uuid;
        }
    }

    return {};
}
