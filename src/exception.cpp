#include <rabbit/exception.hpp>

#include <iostream>

using namespace rb;

exception::exception(const std::string& message)
    : runtime_error(message) {
}
