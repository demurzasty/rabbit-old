#include <rabbit/assets.hpp>

#include <fstream>

using namespace rb;

std::unordered_map<std::type_index, assets::loader> assets::_loaders;
std::map<uuid, std::weak_ptr<void>> assets::_assets;

void assets::init() {
}

void assets::release() {
	_assets.clear();
	_loaders.clear();
}

std::shared_ptr<void> assets::_load(std::type_index type_index, const uuid& uuid) {
    auto& asset = _assets[uuid];
    if (!asset.expired()) {
        return asset.lock();
    }

    bstream stream{ "package/" + uuid.to_string(), bstream_mode::read };
    // TODO: Test filetype.

    auto& loader = _loaders.at(type_index);
    auto loaded_asset = loader(stream);
    asset = loaded_asset;
    return loaded_asset;
}

json assets::_load_metadata(const std::string& filename) {
    json metadata;

    std::ifstream stream{ filename + ".meta", std::ios::in };
    if (stream.is_open()) {
        stream >> metadata;
    }

    return metadata;
}
