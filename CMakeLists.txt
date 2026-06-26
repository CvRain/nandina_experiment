cmake_minimum_required(VERSION 4.0)
project(nandina_experiment)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    # MSVC: CMake doesn't yet map CXX26 for MSVC, lower standard check
    # to 23 which CMake knows, then override with /std:c++latest for C++26 features
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED OFF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /Zc:__cplusplus")
else()
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Find Conan dependencies
find_package(ZLIB REQUIRED)
find_package(Freetype REQUIRED)
find_package(HarfBuzz REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

# Link Conan dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    ZLIB::ZLIB
    Freetype::Freetype
    HarfBuzz::HarfBuzz
)
