#pragma once 

#include "../importer.hpp"

namespace rb {
    class texture_metadata : public metadata {
    public:

    };

    class texture_importer : public importer {
    public:
        void import(const std::string_view& input, const std::string_view& output, const json& metadata) override;

        std::shared_ptr<metadata> make_metadata() const override;

        span<const char*> supported_extensions() const override;
    };
}
