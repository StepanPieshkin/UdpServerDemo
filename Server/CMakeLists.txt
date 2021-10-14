cmake_minimum_required (VERSION 3.8)

set(ASIO_NO_DEPRECATED TRUE)
add_compile_options(/permissive- /Zc:externConstexpr)

find_package(boost REQUIRED COMPONENTS asio)
include_directories(${Boost_INCLUDE_DIRS})

add_executable (Server "Server.cxx")
target_link_libraries(Server ${Boost_LIBRARIES})