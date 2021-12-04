#pragma once 

#include "../core/span.hpp"
#include "../core/json.hpp"

#include <string_view>

namespace rb {
    class metadata {
    public:
        virtual ~metadata() = default;
    };

    class importer {
    public:
        virtual ~importer() = default;

        virtual void import(const std::string_view& input, const std::string_view& output, const json& metadata) = 0;

        virtual std::shared_ptr<metadata> make_metadata() const = 0;

        virtual span<const char*> supported_extensions() const = 0;
    };
}
