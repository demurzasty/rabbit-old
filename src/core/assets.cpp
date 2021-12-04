#include <rabbit/core/assets.hpp>
#include <rabbit/core/config.hpp>

// TODO: We shouldn't depend on C++17 filesystem on production.
//       Some of consoles doesn't support it.
#include <filesystem>

using namespace rb;

asset_storage::asset_storage(const std::shared_ptr<void>& ptr)
    : _ptr(ptr) {
}

void asset_storage::reload(const std::shared_ptr<void>& ptr) {
    _ptr = ptr;
    _reload.publish();
}

const std::shared_ptr<void>& asset_storage::ptr() const noexcept {
    return _ptr;
}

sink<void()> asset_storage::on_reload() noexcept {
    return { _reload };
}

std::shared_ptr<asset_storage> assets_impl::_load(id_type asset_type_id, const uuid& uuid) {
    auto& asset = _assets[uuid];
    if (!asset.expired()) {
        return asset.lock();
    }

    auto& loader = _loaders[asset_type_id];
    if (!loader) {
        RB_DEBUG_LOG("Loader for type #{} does not exist.", asset_type_id);
        return nullptr;
    }

    const auto filename = format("package/{}", uuid.to_string());
    if (!std::filesystem::exists(filename)) {
        RB_DEBUG_LOG("File \"{}\" does not exist.", filename);
        return nullptr;
    }

    const auto loaded_asset = std::make_shared<asset_storage>(loader->load(filename));
    asset = loaded_asset;
    return loaded_asset;
}

std::unique_ptr<assets_impl> assets::_impl;

void assets::setup() {
    _impl = std::make_unique<assets_impl>();
}

void assets::release() {
    _impl.reset();
}
