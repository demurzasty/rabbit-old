cmake_minimum_required (VERSION 3.8.2)

add_library (quickhull STATIC "QuickHull.cpp")
target_include_directories (quickhull PUBLIC ".")
