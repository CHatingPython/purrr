cmake_minimum_required(VERSION 3.14)

include(CMakePackageConfigHelpers)

project(purrr CXX)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SRCS "src/**.cpp")
file(GLOB_RECURSE HDRS "inc/**.hpp")
add_library(purrr STATIC ${SRCS} ${HDRS})
target_include_directories(purrr
  PRIVATE src/
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(purrr Vulkan::Vulkan)

target_compile_definitions(purrr PRIVATE _PURRR_PLATFORM=win32 _PURRR_BACKEND_VULKAN)

add_executable(example example/main.cpp)
target_link_libraries(example purrr -static)

# Install
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/purrrVersion.cmake"
  VERSION 0.0.0
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/purrrConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/purrrConfig.cmake"
  INSTALL_DESTINATION lib/cmake/purrr
)

install(TARGETS purrr
  EXPORT purrrTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY inc/ DESTINATION include)

install(EXPORT purrrTargets
  FILE purrrTargets.cmake
  NAMESPACE purrr::
  DESTINATION lib/cmake/purrr
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/purrrConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/purrrVersion.cmake"
  DESTINATION lib/cmake/purrr
)