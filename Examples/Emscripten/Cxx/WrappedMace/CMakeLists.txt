cmake_minimum_required(VERSION 3.13)
project(WrappedMace)

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

add_executable(WrappedMace WrappedMace.cxx)
target_link_libraries(WrappedMace PRIVATE ${VTK_LIBRARIES})


# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_link_options)

list(APPEND emscripten_link_options
  "-lembind"
  "SHELL:-s WASM=1"
  "SHELL:-s EXPORT_NAME=createWrappedMaceModule"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s ENVIRONMENT=web"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
)

target_link_options(WrappedMace
  PUBLIC
    ${emscripten_link_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  WrappedMace
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET WrappedMace
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:WrappedMace>
)
