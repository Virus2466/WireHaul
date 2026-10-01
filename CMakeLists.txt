cmake_minimum_required(VERSION 3.20)

# Project name
project(Wirehaul CXX)

# Force C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add strict warning flags (These will make you a better coder)
if(MSVC)
    # MSVC (Windows) strict flags
    add_compile_options(/W4 /permissive-)
else()
    # GCC/Clang (Linux/WSL) strict flags
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add your executable
add_executable(wirehaul 
       src/main.cpp
       src/Bencode.cpp
)

# If you use CMake with WSL, it will automatically apply the GCC flags!

target_include_directories(wirehaul PRIVATE include)