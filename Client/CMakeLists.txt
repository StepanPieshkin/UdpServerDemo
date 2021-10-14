cmake_minimum_required (VERSION 3.8)

set(ASIO_NO_DEPRECATED TRUE)
find_package(asio CONFIG REQUIRED)

add_executable (Client "Client.cxx")
target_link_libraries(Client asio)
