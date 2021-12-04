#include <rabbit/rabbit.hpp>

using namespace rb;

int main(int argc, char* argv[]) {
#if !RB_PROD_BUILD
    settings::app.data_directory = DATA_DIRECTORY;
#endif

    app::setup();

    app::run();
}

