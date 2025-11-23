cmake_minimum_required(VERSION 3.14)

include(CMakePackageConfigHelpers)

project(purrr CXX)

option(PURRR_VULKAN_SUPPORT "Enable Vulkan support" ON)

set(PURRR_LIBRARIES "")
set(PURRR_INCLUDES "")
set(PURRR_DEFINES "")

# 
# Platform checks
# 
if(WIN32)
  message(STATUS "Platform: Windows")
  list(APPEND PURRR_DEFINES _PURRR_PLATFORM_WIN32)
elseif(UNIX AND NOT APPLE)
  message(STATUS "Platform: Linux")
  list(APPEND PURRR_DEFINES _PURRR_PLATFORM_LINUX)

  find_package(X11)

  option(PURRR_X11_SUPPORT "Enable X11 support" ${X11_FOUND})
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

# 
# Backend: Vulkan
# 
if(PURRR_VULKAN_SUPPORT)
  find_package(Vulkan REQUIRED)
  list(APPEND PURRR_LIBRARIES Vulkan::Vulkan)
  list(APPEND PURRR_DEFINES _PURRR_BACKEND_VULKAN)
endif()

# 
# Platform: X11
# 
if(PURRR_X11_SUPPORT)
  if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "X11 can only be enabled on Linux.")
  elseif(NOT X11_FOUND)
    message(FATAL_ERROR "X11 has not been found.")
  endif()

  list(APPEND PURRR_LIBRARIES ${X11_LIBRARIES})
  list(APPEND PURRR_INCLUDES ${X11_INCLUDE_DIR})

  list(APPEND PURRR_DEFINES _PURRR_PLATFORM_X11)
endif()

# 
# purrr
# 
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SRCS "src/**.cpp")
file(GLOB_RECURSE HDRS "inc/**.hpp")
add_library(purrr STATIC ${SRCS} ${HDRS})
target_include_directories(purrr
  PRIVATE src/
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>)

target_compile_options(purrr PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_link_libraries(purrr PRIVATE ${PURRR_LIBRARIES})
target_include_directories(purrr PRIVATE ${PURRR_INCLUDES})
target_compile_definitions(purrr PRIVATE ${PURRR_DEFINES})

# 
# Example
# 
add_executable(example example/main.cpp)
target_link_libraries(example PRIVATE purrr)
if(MINGW)
  target_link_libraries(example PRIVATE -static)
endif()

# 
# Install
# 
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