#include <rabbit/rabbit.hpp>

#include <iostream>

using namespace rb;

std::unordered_map<std::string, importer> editor::_importers;

void editor::init() {
	add_importer<texture>(".png", ".bmp", ".jpg");
	add_importer<mesh>(".obj");
	add_importer<material>(".mat");
	add_importer<environment>(".env");
}

void editor::release() {
	_importers.clear();
}

void editor::scan() {
	for (auto& dir_entry : std::filesystem::recursive_directory_iterator{ "./data" }) {
		std::cout << dir_entry.path() << std::endl;
	}
}
