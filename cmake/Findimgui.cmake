# ${CMAKE_SOURCE_DIR}/cmake/FindImgui.cmake
if(TARGET imgui::SDL_OpenGL)
  return()
endif()

find_package(SDL2      REQUIRED)
find_package(OpenGL    REQUIRED)
find_package(glbinding REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui/)

add_library(
  imgui_sdl_opengl
  STATIC
)

target_sources(
  imgui_sdl_opengl
  PRIVATE
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
)

target_link_libraries(
  imgui_sdl_opengl
  PUBLIC
  OpenGL::GL
  SDL2::SDL2
  glbinding::glbinding
)

target_include_directories(
  imgui_sdl_opengl
  SYSTEM PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends/
)

add_library(
  imgui::sdl2
  ALIAS
  imgui_sdl_opengl
)

target_compile_options(
  imgui_sdl_opengl
  PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-w>
  $<$<CXX_COMPILER_ID:GNU>:-w>
)

target_compile_definitions(
  imgui_sdl_opengl
  PUBLIC
  IMGUI_IMPL_OPENGL_LOADER_GLBINDING3
)

