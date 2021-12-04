#include <rabbit/editor/editor.hpp>
#include <rabbit/core/settings.hpp>
#include <rabbit/core/config.hpp>

#include <filesystem>

using namespace rb;

std::unique_ptr<editor_impl> editor::_impl;

void editor_impl::scan() {
    // TODO: Travese and check only differences.
    _root_entry = make_entry(settings::app.data_directory);
    scan(_root_entry);
}

void editor_impl::scan(const std::shared_ptr<entry>& entry) {
    if (!std::filesystem::is_directory(entry->path)) {
        RB_DEBUG_LOG("Cannot perform scan on non-directory entry: {}.", entry->path.string());
        return;
    }

    for (const auto& dir_entry : std::filesystem::directory_iterator(entry->path)) {
        if (!dir_entry.is_directory() && !dir_entry.is_regular_file()) {
            continue;
        }

        const auto extension = dir_entry.path().extension();
        if (extension == ".meta") {
            continue;
        }

        if (dir_entry.is_directory() && extension == ".data") {
            continue;
        }

        auto child = make_entry(dir_entry.path(), entry);

        if (dir_entry.is_directory()) {
            scan(child);
        } else {
            child->importer = _find_suitable_importer(extension.string());
            if (!child->importer) {
                RB_DEBUG_LOG("No suitable importer found for extension: {}", extension.string());
            }
        }

        entry->children.push_back(child);
    }
}

std::shared_ptr<importer> editor_impl::_find_suitable_importer(const std::string_view& extension) {
    for (const auto& [asset_type_id, importer] : _importers) {
        for (const auto& supported_extension : importer->supported_extensions()) {
            if (supported_extension == extension) {
                return importer;
            }
        }
    }
    return nullptr;
}

void editor::setup() {
    _reflect();

    _impl = std::make_unique<editor_impl>();
}

void editor::release() {
    _impl.reset();
}

void editor::scan() {
    _impl->scan();
}
