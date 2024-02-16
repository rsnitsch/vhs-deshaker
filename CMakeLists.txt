cmake_minimum_required(VERSION 3.12)

project(vhs-deshaker VERSION 1.0.0)

find_package(OpenCV REQUIRED)

include_directories("include")
include_directories("dependencies")
add_subdirectory(src)
