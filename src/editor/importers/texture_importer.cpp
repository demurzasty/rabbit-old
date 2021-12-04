#include <rabbit/editor/importers/texture_importer.hpp>

using namespace rb;

void texture_importer::import(const std::string_view& input, const std::string_view& output, const json& metadata) {
}

std::shared_ptr<metadata> texture_importer::make_metadata() const {
    return std::make_shared<texture_metadata>();
}

span<const char*> texture_importer::supported_extensions() const {
    static const char* extensions[]{ ".png", ".jpg", ".bmp" };
    return extensions;
}
