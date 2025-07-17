cmake_minimum_required(VERSION 3.29)
project(GeometryViewer)

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
    CommonColor
    FiltersGeometry
    InteractionStyle
    InteractionWidgets
    IOPLY
    IOGeometry
    IOXML
    RenderingOpenGL2
    RenderingWebGPU
    RenderingUI
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(GeometryViewer
  GeometryViewer.h
  GeometryViewer.cxx
)

target_link_libraries(GeometryViewer
  PRIVATE
    ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_link_options)
set(emscripten_compile_options)

list(APPEND emscripten_link_options
  "-lembind"
  "-sJSPI=1" # for webgpu async APIs
  "-sJSPI_EXPORTS=['__wasm_call_ctors']"
  "-sMODULARIZE=1"
  "-sEXPORT_ES6=1"
  "-sEXPORT_NAME=createGeometryViewerModule"
  "-sENVIRONMENT=web"
  "-sALLOW_MEMORY_GROWTH=1"
  "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/helper.js"
  #"--memoryprofiler"
  #"--cpuprofiler"
  "-sEXPORTED_FUNCTIONS=['_malloc', '_free']"
  "-sEXPORTED_RUNTIME_METHODS=['ENV', 'UTF8ToString', 'HEAPU8']"
)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=16GB")
else ()
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=4GB")
endif ()

# -----------------------------------------------------------------------------
# Optimizations
# -----------------------------------------------------------------------------
set(emscripten_optimizations)
set(emscripten_debug_options)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(geometry_viewer_optimize "BEST")
  set(geometry_viewer_debug_info "NONE")
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(geometry_viewer_optimize "SMALLEST_WITH_CLOSURE")
  set(geometry_viewer_debug_info "NONE")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(geometry_viewer_optimize "MORE")
  set(geometry_viewer_debug_info "PROFILE")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(geometry_viewer_optimize "NO_OPTIMIZATION")
  set(geometry_viewer_debug_info "DEBUG_NATIVE")
endif ()
set(geometry_viewer_optimize_NO_OPTIMIZATION "-O0")
set(geometry_viewer_optimize_LITTLE "-O1")
set(geometry_viewer_optimize_MORE "-O2")
set(geometry_viewer_optimize_BEST "-O3")
set(geometry_viewer_optimize_SMALLEST "-Os")
set(geometry_viewer_optimize_SMALLEST_WITH_CLOSURE "-Oz")
set(geometry_viewer_optimize_SMALLEST_WITH_CLOSURE_link "--closure=1")

if (DEFINED "geometry_viewer_optimize_${geometry_viewer_optimize}")
  list(APPEND emscripten_optimizations
    ${geometry_viewer_optimize_${geometry_viewer_optimize}})
  list(APPEND emscripten_link_options
    ${geometry_viewer_optimize_${geometry_viewer_optimize}_link})
else ()
  message (FATAL_ERROR "Unrecognized value for geometry_viewer_optimize=${geometry_viewer_optimize}")
endif ()

set(geometry_viewer_debug_info_NONE "-g0")
set(geometry_viewer_debug_info_READABLE_JS "-g1")
set(geometry_viewer_debug_info_PROFILE "-g2")
set(geometry_viewer_debug_info_DEBUG_NATIVE "-g3")
set(geometry_viewer_debug_info_DEBUG_NATIVE_link "-sASSERTIONS=1")
if (DEFINED "geometry_viewer_debug_info_${geometry_viewer_debug_info}")
  list(APPEND emscripten_debug_options
    ${geometry_viewer_debug_info_${geometry_viewer_debug_info}})
  list(APPEND emscripten_link_options
    ${geometry_viewer_debug_info_${geometry_viewer_debug_info}_link})
else ()
  message (FATAL_ERROR "Unrecognized value for geometry_viewer_debug_info=${geometry_viewer_debug_info}")
endif ()

target_compile_options(GeometryViewer
  PUBLIC
    ${emscripten_compile_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
)

target_link_options(GeometryViewer
  PUBLIC
    ${emscripten_link_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  GeometryViewer
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------
add_custom_command(
  TARGET GeometryViewer
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/web"
      "${CMAKE_CURRENT_BINARY_DIR}"
)
