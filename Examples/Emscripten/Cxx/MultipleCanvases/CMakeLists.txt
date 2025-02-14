cmake_minimum_required(VERSION 3.13)
project(MultipleCanvases)

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
    RenderingUI         # For window
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(MultipleCanvases MultipleCanvases.cxx)
target_link_libraries(MultipleCanvases PRIVATE ${VTK_LIBRARIES})

# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_link_options)
list(APPEND emscripten_link_options
  "-sALLOW_MEMORY_GROWTH=1"
  "-sSINGLE_FILE=1"
  "-sEXPORTED_RUNTIME_METHODS=['ENV']" # ENV holds the environment variables accessible by C getenv
  "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/index.html"
)
if (CMAKE_SIZEOF_VOID_P EQUAL "8")
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=16GB")
else ()
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=4GB")
endif ()
target_link_options(MultipleCanvases
  PUBLIC
    ${emscripten_link_options}
)
set_target_properties(MultipleCanvases
PROPERTIES 
  OUTPUT_NAME "index"
  SUFFIX ".html")

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  MultipleCanvases
  MODULES  ${VTK_LIBRARIES}
)
