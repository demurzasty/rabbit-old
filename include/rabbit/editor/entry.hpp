#pragma once 

#include "importer.hpp"

#include <memory>
#include <vector>
#include <filesystem>

namespace rb {
    struct entry : public std::enable_shared_from_this<entry> {
        std::filesystem::path path;
        std::shared_ptr<entry> parent;
        std::vector<std::shared_ptr<entry>> children;
        std::shared_ptr<importer> importer;
    };

    inline std::shared_ptr<entry> make_entry(const std::filesystem::path& path, std::shared_ptr<entry> parent = nullptr) {
        auto entry = std::make_shared<rb::entry>();
        entry->path = path;
        entry->parent = parent;
        return entry;
    }
}
