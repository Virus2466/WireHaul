cmake_minimum_required(VERSION 3.20)
project(Wirehaul CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(CURL REQUIRED)
find_package(asio CONFIG REQUIRED)

add_executable(wirehaul
    src/main.cpp
    src/Bencode.cpp
    src/Sha1.cpp
    src/Metainfo.cpp
    src/Tracker.cpp
    src/PeerWire.cpp
)
target_include_directories(wirehaul PRIVATE include)
target_link_libraries(wirehaul PRIVATE CURL::libcurl asio::asio)
target_compile_definitions(wirehaul PRIVATE _WIN32_WINNT=0x0A00 NOMINMAX)

enable_testing()

add_executable(bencode_tests tests/bencode_tests.cpp src/Bencode.cpp)
target_include_directories(bencode_tests PRIVATE include)
add_test(NAME bencode COMMAND bencode_tests)

add_executable(sha1_tests tests/sha1_tests.cpp src/Sha1.cpp)
target_include_directories(sha1_tests PRIVATE include)
add_test(NAME sha1 COMMAND sha1_tests)


add_executable(peerwire_tests tests/peerwire_tests.cpp src/PeerWire.cpp)
target_include_directories(peerwire_tests PRIVATE include)
target_link_libraries(peerwire_tests PRIVATE asio::asio)
target_compile_definitions(peerwire_tests PRIVATE _WIN32_WINNT=0x0A00 NOMINMAX)
add_test(NAME peerwire COMMAND peerwire_tests)