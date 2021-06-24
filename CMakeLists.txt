# ${CMAKE_SOURCE_DIR}/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(GdbImgui CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(options)

find_package(fmt       REQUIRED)
find_package(SDL2      REQUIRED)
find_package(glbinding REQUIRED)
find_package(imgui     REQUIRED)
find_package(OpenGL    REQUIRED)

add_executable(
  ${PROJECT_NAME}
)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  main.cpp
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  fmt::fmt
  imgui::sdl2
  options::options
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  CXX_STANDARD          17
  CXX_EXTENSIONS        OFF
  CXX_STANDARD_REQUIRED ON
)

