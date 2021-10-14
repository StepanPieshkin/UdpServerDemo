# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("ClientServerDemo")

# Include sub-projects.
add_subdirectory ("Server")
add_subdirectory ("Client")
add_subdirectory ("Test")
