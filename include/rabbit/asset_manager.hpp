#pragma once 

#include "type_info.hpp"
#include "di.hpp"
#include "loader.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace rb {
    class asset_manager {
    public:
        asset_manager(injector& injector);

        template<typename Asset, typename Loader, typename... Extensions>
        void add_loader(Extensions&&... extensions) {
            auto loader = _injector.resolve<Loader>();
            (_loaders.emplace(extensions, loader), ...);
        }
    
    private:
        injector _injector;
        std::unordered_map<std::string, std::shared_ptr<loader>> _loaders;
    };
}