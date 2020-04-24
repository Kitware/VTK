cmake_minimum_required(VERSION 3.13)
project(MultiCone)

# -----------------------------------------------------------------------------
# EMSCRIPTEN only
# -----------------------------------------------------------------------------

if (NOT EMSCRIPTEN)
  message("Skipping example: This needs to run inside an Emscripten build environment")
  return ()
endif ()

# -----------------------------------------------------------------------------
# Handle VTK dependency
# -----------------------------------------------------------------------------

find_package(VTK
  COMPONENTS
    FiltersSources      # VTK pipeline
    InteractionStyle    # Mouse handling
    RenderingOpenGL2    # For Rendering
    RenderingUI         # For SDL2 Window
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# -----------------------------------------------------------------------------
# WebAssembly build options
# (-s) https://github.com/emscripten-core/emscripten/blob/master/src/settings.js
# -----------------------------------------------------------------------------

set(emscripten_options)
list(APPEND emscripten_options
  "--bind"
  "-g3"
  "-O3"
  "SHELL:-s EXPORT_NAME=vtkCone"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s DEMANGLE_SUPPORT=1"
  "SHELL:-s EMULATE_FUNCTION_POINTER_CASTS=0"
  "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0"
  "SHELL:-s USE_PTHREADS=0"
  "SHELL:-s WASM=1"
  "SHELL:-s EXPORTED_FUNCTIONS=\"['_main', '_stop', '_getConeResolution', '_setConeResolution']\""
  "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=\"['ccall', 'cwrap']\""
  "SHELL:-s ENVIRONMENT=web"
)

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(MultiCone MultiCone.cxx)

target_link_libraries(MultiCone
  PRIVATE
    VTK::FiltersSources
    VTK::InteractionStyle
    VTK::RenderingOpenGL2
    VTK::RenderingUI
)

target_compile_options(MultiCone
  PUBLIC
    ${emscripten_options}
)

target_link_options(MultiCone
  PUBLIC
    ${emscripten_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  MultiCone
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET MultiCone
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:MultiCone>
)
