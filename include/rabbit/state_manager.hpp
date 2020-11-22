#pragma once 

#include "state.hpp"

#include <string>
#include <memory>
#include <unordered_map>

namespace rb {
    class state_manager {
    public:
        void add(const std::string& name, std::shared_ptr<state> state);

        void set(const std::string& name);

        void initialize();

        void release();

        void update(float elapsed_time);

        void fixed_update(float fixed_time);

        void draw();

    private:
        std::shared_ptr<state> _current_state;
        std::unordered_map<std::string, std::shared_ptr<state>> _states;
    };
}