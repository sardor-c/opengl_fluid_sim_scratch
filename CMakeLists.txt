cmake_minimum_required(VERSION 3.24)
project(fluid_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW topilmadi, GLFW 3.4 yuklab olinadi")
    set(GLFW_BUILD_DOCS OFF)
    include(FetchContent)
    FetchContent_Declare(glfw
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
        URL_HASH SHA256=b5ec004b2712fd08e8861dc271428f048775200a2df719ccf575143ba749a3e9)
    FetchContent_MakeAvailable(glfw)
endif()

add_executable(fluid_sim
    src/main.cpp
    src/gl.cpp
    src/shader.cpp
    src/sim2d.cpp
)

target_include_directories(fluid_sim PRIVATE external/khronos)
target_compile_definitions(fluid_sim PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")
target_compile_options(fluid_sim PRIVATE -Wall -Wextra)
target_link_libraries(fluid_sim PRIVATE glfw)