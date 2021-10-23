#include <rabbit/assets.hpp>

#include <fstream>

using namespace rb;

std::unordered_map<std::type_index, assets::loader> assets::_loaders;
std::unordered_map<std::string, std::weak_ptr<void>> assets::_assets;

void assets::init() {
}

void assets::release() {
	_assets.clear();
	_loaders.clear();
}

std::shared_ptr<void> assets::_load(std::type_index type_index, const std::string& filename) {
    auto& asset = _assets[filename];
    if (!asset.expired()) {
        return asset.lock();
    }

    auto& loader = _loaders.at(type_index);
    auto loaded_asset = loader(filename, _load_metadata(filename));
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
