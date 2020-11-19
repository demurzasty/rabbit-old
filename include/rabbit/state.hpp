#pragma once 

namespace rb {
    class state {
    public:
        virtual ~state() = default;

        virtual void update(float elapsed_time);

        virtual void fixed_update(float fixed_time);

        virtual void draw();
    };
}