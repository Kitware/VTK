cmake_minimum_required(VERSION 3.13)
project(Cone)

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
# -----------------------------------------------------------------------------

set(emscripten_options)
list(APPEND emscripten_options
  "--bind"
  "-g3"
  "SHELL:-s EXPORT_NAME=vtkApp"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s DEMANGLE_SUPPORT=1"
  "SHELL:-s EMULATE_FUNCTION_POINTER_CASTS=0"
  "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s USE_PTHREADS=0"
  "SHELL:-s WASM=1"
)

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------

set(OPTIMIZE "BEST" CACHE STRING "Emscripten optimization")
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
  # Cone.js    659K
  # Cone.wasm  4.9M
  # time => 4 minutes 3 seconds
  list(APPEND emscripten_options
    "-Oz"
  )
elseif(OPTIMIZE STREQUAL "LITTLE")
  # Cone.js    529K
  # Cone.wasm  5.9M
  list(APPEND emscripten_options
    "-O1"
  )
elseif(OPTIMIZE STREQUAL "MORE")
  # Cone.js    529K
  # Cone.wasm  5.3M
  list(APPEND emscripten_options
    "-O2"
  )
elseif(OPTIMIZE STREQUAL "BEST")
  # Cone.js    529K
  # Cone.wasm  4.9M
  # time => 4 minutes 7 seconds
  list(APPEND emscripten_options
    "-O3"
  )
elseif(OPTIMIZE STREQUAL "SMALL")
  # Cone.js    529K
  # Cone.wasm  4.9M
  list(APPEND emscripten_options
    "-Os"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST")
  # Cone.js    659K
  # Cone.wasm  4.9M
  list(APPEND emscripten_options
    "-Oz"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST_WITH_CLOSURE")
  # Cone.js    659K
  # Cone.wasm  4.9M
  list(APPEND emscripten_options
    "-Oz"
    "SHELL:--closure 1"
  )
endif()

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(Cone Cone.cxx)

target_link_libraries(Cone
  PRIVATE
    VTK::FiltersSources
    VTK::InteractionStyle
    VTK::RenderingOpenGL2
    VTK::RenderingUI
)

target_compile_options(Cone
  PUBLIC
    ${emscripten_options}
)

target_link_options(Cone
  PUBLIC
    ${emscripten_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  Cone
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET Cone
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:Cone>
)
