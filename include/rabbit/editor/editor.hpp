#pragma once 

#include "entry.hpp"
#include "importer.hpp"
#include "../core/type.hpp"

#include <memory>
#include <filesystem>

namespace rb {
    class editor_impl {
    public:
        void scan();

    private:
        void scan(const std::shared_ptr<entry>& entry);

        std::shared_ptr<importer> _find_suitable_importer(const std::string_view& extension);

    private:
        std::shared_ptr<entry> _root_entry;
        std::unordered_map<id_type, std::shared_ptr<importer>> _importers;
    };

    class editor {
    public:
        static void setup();

        static void release();

        static void scan();

    private:
        static void _reflect();

    private:
        static std::unique_ptr<editor_impl> _impl;
    };
}
