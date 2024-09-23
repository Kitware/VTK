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
# Compile example code
# -----------------------------------------------------------------------------

add_executable(MultiCone MultiCone.cxx)
target_link_libraries(MultiCone PRIVATE ${VTK_LIBRARIES})

# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_link_options)

list(APPEND emscripten_link_options
  "-lembind"
  "SHELL:-s WASM=1"
  "SHELL:-s EXPORT_NAME=createMultiConeModule"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s EXPORTED_FUNCTIONS=\"['_main', '_stop', '_getConeResolution', '_setConeResolution']\""
  "SHELL:-s EXPORTED_RUNTIME_METHODS=\"['ccall', 'cwrap']\""
  "SHELL:-s ENVIRONMENT=web"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
)

target_link_options(MultiCone
  PUBLIC
    ${emscripten_link_options}
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
