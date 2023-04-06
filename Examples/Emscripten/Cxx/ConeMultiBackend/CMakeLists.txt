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

target_link_libraries(ConeMultiBackend
  PRIVATE
    VTK::FiltersSources
    VTK::InteractionStyle
    VTK::RenderingOpenGL2
    VTK::RenderingWebGPU
    VTK::RenderingUI
)

# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_link_options)
set(emscripten_compile_options)

list(APPEND emscripten_link_options
  "-sWASM=1"
  "-sMODULARIZE=1"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sEXPORT_NAME=createConeMultiBackendModule"
  "-sEXPORTED_RUNTIME_METHODS=['ENV']" # ENV holds the environment variables accessible by C getenv
)

set(emscripten_debug_options)
set(DEBUGINFO "PROFILE" CACHE STRING "Type of debug info")
set_property(CACHE DEBUGINFO PROPERTY
  STRINGS
    NONE              # -g0
    READABLE_JS       # -g1
    PROFILE           # -g2
    DEBUG_NATIVE      # -g3
)

if(DEBUGINFO STREQUAL "NONE")
  list(APPEND emscripten_debug_options
    "-g0"
  )
elseif(DEBUGINFO STREQUAL "READABLE_JS")
  list(APPEND emscripten_debug_options
    "-g1"
  )
  list(APPEND emscripten_link_options
    "-sDEMANGLE_SUPPORT=1"
  )
elseif(DEBUGINFO STREQUAL "PROFILE")
  list(APPEND emscripten_debug_options
    "-g2"
  )
  list(APPEND emscripten_link_options
    "-sDEMANGLE_SUPPORT=1"
  )
elseif(DEBUGINFO STREQUAL "DEBUG_NATIVE")
  list(APPEND emscripten_debug_options
    "-g3"
  )
  list(APPEND emscripten_link_options
    "-sASSERTIONS=1"
    "-sDEMANGLE_SUPPORT=1"
  )
endif()

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------
set(emscripten_optimizations)
set(OPTIMIZE "SMALLEST_WITH_CLOSURE" CACHE STRING "Emscripten optimization")
set_property(CACHE OPTIMIZE PROPERTY
  STRINGS
    NO_OPTIMIZATION       # -O0
    LITTLE                # -O1
    MORE                  # -O2
    BEST                  # -O3
    SMALL                 # -Os
    SMALLEST              # -Oz
    SMALLEST_WITH_CLOSURE # -Oz --closure 1
)

if(OPTIMIZE STREQUAL "NO_OPTIMIZATION")
  list(APPEND emscripten_optimizations
    "-O0"
  )
elseif(OPTIMIZE STREQUAL "LITTLE")
  list(APPEND emscripten_optimizations
    "-O1"
  )
elseif(OPTIMIZE STREQUAL "MORE")
  list(APPEND emscripten_optimizations
    "-O2"
  )
elseif(OPTIMIZE STREQUAL "BEST")
  list(APPEND emscripten_optimizations
    "-O3"
  )
elseif(OPTIMIZE STREQUAL "SMALL")
  list(APPEND emscripten_optimizations
    "-Os"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST")
  list(APPEND emscripten_optimizations
    "-Oz"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST_WITH_CLOSURE")
  list(APPEND emscripten_optimizations
    "-Oz"
  )
  list(APPEND emscripten_link_options
    "--closure 1"
  )
endif()

target_compile_options(ConeMultiBackend
  PUBLIC
    ${emscripten_compile_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
)

target_link_options(ConeMultiBackend
  PUBLIC
    ${emscripten_link_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
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
