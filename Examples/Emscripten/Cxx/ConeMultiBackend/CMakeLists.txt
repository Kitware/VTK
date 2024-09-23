cmake_minimum_required(VERSION 3.13)
project(ConeMultiBackend)

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
    RenderingOpenGL2    # For Rendering with OpenGL
    RenderingWebGPU     # For Rendering with WebGPU
    RenderingUI         # For SDL2 Window
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(ConeMultiBackend ConeMultiBackend.cxx)
target_link_libraries(ConeMultiBackend PRIVATE ${VTK_LIBRARIES})

# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_link_options)

list(APPEND emscripten_link_options
  "-sWASM=1"
  "-sMODULARIZE=1"
  "-sASYNCIFY=1"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sEXPORT_NAME=createConeMultiBackendModule"
  "-sEXPORTED_RUNTIME_METHODS=['ENV']" # ENV holds the environment variables accessible by C getenv
)

target_link_options(ConeMultiBackend
  PUBLIC
    ${emscripten_link_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  ConeMultiBackend
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET ConeMultiBackend
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/web"
      "${CMAKE_CURRENT_BINARY_DIR}"
)
