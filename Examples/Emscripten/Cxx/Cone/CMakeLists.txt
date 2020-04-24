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
    # CommonDataModel
    # FiltersSources
    # InteractionStyle
    # RenderingCore
    # RenderingOpenGL2

    FiltersSources
    InteractionStyle
    RenderingUI
    RenderingOpenGL2
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# -----------------------------------------------------------------------------
# This should become optional once we attach proper flag to VTK targets
# -----------------------------------------------------------------------------

# WebAssembly export name: "vtkApp"
# set(emscripten_options "SHELL:-s EXPORT_NAME=vtkApp")

# # Add WebAssembly build options
# list(APPEND emscripten_options
#   "SHELL:-s USE_PTHREADS=0"
#   "SHELL:-s ALLOW_MEMORY_GROWTH=1"
#   "SHELL:-s USE_FREETYPE=1"
#   "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0"
#   "SHELL:-s DEMANGLE_SUPPORT=1"
#   "SHELL:-s WASM=1"
#   "SHELL:-s EMULATE_FUNCTION_POINTER_CASTS=0"
#   "SHELL:-s MODULARIZE=1"
#   "SHELL:-s FORCE_FILESYSTEM=1"
#   "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=[\"FS\"]"
# )

# ----------------------------------------------------
# Handled by FindOpenGL of VTK 9
# ----------------------------------------------------
# if(EMSCRIPTEN_VERSION VERSION_GREATER_EQUAL 1.39.5)
#   list(APPEND emscripten_options
#     "SHELL:-s MIN_WEBGL_VERSION=2"
#     "SHELL:-s MAX_WEBGL_VERSION=2"
#   )
# else()
#   list(APPEND emscripten_options
#     "SHELL:-s USE_WEBGL2=1"
#   )
# endif()
# ----------------------------------------------------

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(Cone Cone.cxx)
target_link_libraries(Cone
  PRIVATE
    VTK::FiltersSources
    VTK::InteractionStyle
    VTK::RenderingUI
    VTK::RenderingOpenGL2
)

# set_target_properties(Cone
#   PROPERTIES
#     INTERFACE_COMPILE_OPTIONS "${emscripten_options}"
#     INTERFACE_LINK_OPTIONS "${emscripten_options}"
# )


# TODO: Clean that part...
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 --bind -g3 -O2 \
  -s EXPORT_NAME=vtkApp \
  -s USE_PTHREADS=0 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s USE_FREETYPE=1 \
  -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
  -s DEMANGLE_SUPPORT=1 \
  -s WASM=1 \
  -s EMULATE_FUNCTION_POINTER_CASTS=0 \
  -s MODULARIZE=1 \
  -s FORCE_FILESYSTEM=1 \
  -s EXTRA_EXPORTED_RUNTIME_METHODS=[\"FS\"] \
")


# Properly initialize our VTK modules
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
      "${CMAKE_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:Cone>
)
