#pragma once 

#include "fnv1a.hpp"
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
            _loaders.emplace(typeid(Asset), loader);
        }

        template<typename Asset>
        std::shared_ptr<Asset> load(const std::string& filename) {
            auto& asset = _assets[filename];
            if (!asset) {
                auto loader = _loaders.at(typeid(Asset));
                asset = loader->load(filename);
            }
            return std::static_pointer_cast<Asset>(asset);
        }

    private:
        std::unordered_map<std::type_index, std::shared_ptr<loader>> _loaders;
        std::unordered_map<std::string, std::shared_ptr<void>> _assets;
    };
}
