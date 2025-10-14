cmake_minimum_required(VERSION 3.14)

project(purrr)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SRCS "src/**.cpp")
file(GLOB_RECURSE HDRS "inc/**.hpp")
add_library(purrr STATIC ${SRCS} ${HDRS})
target_include_directories(purrr PRIVATE src/ PUBLIC inc/)
target_link_libraries(purrr Vulkan::Vulkan)

target_compile_definitions(purrr PRIVATE _PURRR_PLATFORM=win32 _PURRR_BACKEND_VULKAN)

add_executable(example example/main.cpp)
target_link_libraries(example purrr -static)