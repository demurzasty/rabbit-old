#pragma once 

#include "loader.hpp"

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace rb {
    class asset_manager {
    public:
        template<typename Asset>
        void add_loader(std::shared_ptr<loader> loader) {
            loader->_asset_manager = this;

            _loaders.emplace(typeid(Asset), loader);
        }

        template<typename Asset>
        std::shared_ptr<Asset> load(const std::string& filename) {
            auto& asset = _assets[filename];
            if (!asset.expired()) {
                return std::static_pointer_cast<Asset>(asset.lock());
            }

            auto loader = _loaders.at(typeid(Asset));
            auto loaded_asset = loader->load(filename);
            asset = loaded_asset;
            return std::static_pointer_cast<Asset>(loaded_asset);
        }

        std::string filename(std::shared_ptr<void> asset) const;

    private:
        std::unordered_map<std::type_index, std::shared_ptr<loader>> _loaders; // todo: should we use unique_ptr instead?
        std::unordered_map<std::string, std::weak_ptr<void>> _assets; // todo; use weak_ptr instead
    };
}
