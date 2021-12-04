#pragma once 

namespace rb {
    class time {
        friend class app;

    public:
        static void setup();

        static void release();

        static float elapsed_time();

        static float fixed_time();

    private:
        static float _elapsed_time;
        static float _fixed_time;
    };
}
