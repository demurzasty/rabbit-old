#include <rabbit/asset_manager.hpp>
#include <rabbit/json.hpp>

#include <fstream>

using namespace rb;

std::string asset_manager::filename(std::shared_ptr<void> asset) const {
    for (const auto& [filename, loaded_asset] : _assets) {
        if (!loaded_asset.expired() && loaded_asset.lock() == asset) {
            return filename;
        }
    }

    return "";
}

std::shared_ptr<void> asset_manager::load(std::type_index asset_id, const std::string& filename) {
    json metadata;
    
    std::ifstream stream{ filename + ".meta" };
    if (stream.is_open()) {
        stream >> metadata;
    }

    auto loader = _loaders.at(asset_id);
    return loader->load(filename, metadata);
}
