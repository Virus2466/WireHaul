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

add_executable(wirehaul
    src/main.cpp
    src/Bencode.cpp
    src/Sha1.cpp

)
target_include_directories(wirehaul PRIVATE include)

enable_testing()

add_executable(bencode_tests tests/bencode_tests.cpp src/Bencode.cpp)
target_include_directories(bencode_tests PRIVATE include)
add_test(NAME bencode COMMAND bencode_tests)