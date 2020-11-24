#pragma once 

namespace rb {
    class state_manager;

    class state {
        friend class state_manager;

    public:
        virtual ~state() = default;

        virtual void initialize();

        virtual void release();

        virtual void update(float elapsed_time);

        virtual void fixed_update(float fixed_time);

        virtual void draw();

    protected:
        rb::state_manager* state_manager() const;

    private:
        rb::state_manager* _state_manager = nullptr;
    };
}