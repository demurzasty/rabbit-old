#include <rabbit/asset_manager.hpp>

using namespace rb;

std::string asset_manager::filename(std::shared_ptr<void> asset) const {
    for (const auto& [filename, loaded_asset] : _assets) {
        if (!loaded_asset.expired() && loaded_asset.lock() == asset) {
            return filename;
        }
    }

    return "";
}
