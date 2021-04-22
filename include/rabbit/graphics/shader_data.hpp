#pragma once

#include "shader.hpp"
#include "buffer.hpp"
#include "texture.hpp"

#include <map>
#include <memory>
#include <variant>

namespace rb {
    struct shader_data_desc {
        std::shared_ptr<shader> shader;
        std::map<std::size_t, std::variant<std::shared_ptr<buffer>, std::shared_ptr<texture>>> data;
    };

    class shader_data {
    public:
        shader_data(const shader_data_desc& desc);

        ~shader_data() = default;

        virtual void bind_resource(std::size_t slot, const std::shared_ptr<buffer>& buffer) = 0;

        virtual void bind_resource(std::size_t slot, const std::shared_ptr<texture>& buffer) = 0;

        // virtual void set_parameter(const std::string& name, const shader_parameter& value) = 0;

        RB_NODISCARD const std::shared_ptr<shader>& shader() const RB_NOEXCEPT;

    private:
        std::shared_ptr<rb::shader> _shader;
    };
}
