cmake_minimum_required(VERSION 3.13)
project(WrappedAsyncClipper)

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
find_package(VTK REQUIRED)

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------
add_executable(WrappedAsyncClipper WrappedAsyncClipper.cxx WrappedAsyncClipper.h)
target_compile_features(WrappedAsyncClipper PRIVATE cxx_std_14) # for initialized lambda captures

set(vtk_libs_used
  VTK::CommonColor
  VTK::FiltersGeneral
  VTK::FiltersSources
  VTK::InteractionStyle
  VTK::InteractionWidgets
  VTK::RenderingOpenGL2
  VTK::RenderingUI)

target_link_libraries(WrappedAsyncClipper
  PRIVATE
    ${vtk_libs_used}
)

# -----------------------------------------------------------------------------
# Emscripten compile+link options
# -----------------------------------------------------------------------------
set(emscripten_link_options)
list(APPEND emscripten_link_options
  "-lembind"
  "-pthread"
  # Enable SMP after https://gitlab.kitware.com/vtk/vtk/-/issues/19424 is resolved
  #"-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency"
  "-sPTHREAD_POOL_SIZE=1"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sALLOW_TABLE_GROWTH=1"
  "-sEXPORTED_RUNTIME_METHODS=['addFunction', 'ENV']"
  "-sOFFSCREENCANVAS_SUPPORT=1"
  #"-sASSERTIONS=1"
  #"--cpuprofiler"
  #"--memoryprofiler"
  #"--threadprofiler"
  )
if (CMAKE_SIZEOF_VOID_P EQUAL "8")
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=16GB")
else ()
  list(APPEND emscripten_link_options
    "-sMAXIMUM_MEMORY=4GB")
endif ()
list(APPEND emscripten_compile_options
  "-pthread")

# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(emscripten_optimizations)
set(emscripten_debug_options)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(_wasm_optimize "BEST")
  set(_wasm_debuginfo "PROFILE")
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(_wasm_optimize "SMALLEST_WITH_CLOSURE")
  set(_wasm_debuginfo "NONE")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(_wasm_optimize "MORE")
  set(_wasm_debuginfo "PROFILE")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_wasm_optimize "NO_OPTIMIZATION")
  set(_wasm_debuginfo "DEBUG_NATIVE")
endif ()
set(_wasm_optimize_NO_OPTIMIZATION "-O0")
set(_wasm_optimize_LITTLE "-O1")
set(_wasm_optimize_MORE "-O2")
set(_wasm_optimize_BEST "-O3")
set(_wasm_optimize_SMALLEST "-Os")
set(_wasm_optimize_SMALLEST_WITH_CLOSURE "-Oz")
set(_wasm_optimize_SMALLEST_WITH_CLOSURE_link "--closure=1")

if (DEFINED "_wasm_optimize_${_wasm_optimize}")
  list(APPEND emscripten_optimizations
    ${_wasm_optimize_${_wasm_optimize}})
  list(APPEND emscripten_link_options
    ${_wasm_optimize_${_wasm_optimize}_link})
else ()
  message (FATAL_ERROR "Unrecognized value for _wasm_optimize=${_wasm_optimize}")
endif ()

set(_wasm_debuginfo_NONE "-g0")
set(_wasm_debuginfo_READABLE_JS "-g1")
set(_wasm_debuginfo_PROFILE "-g2")
set(_wasm_debuginfo_DEBUG_NATIVE "-g3")
set(_wasm_debuginfo_DEBUG_NATIVE_link "-sASSERTIONS=1")
if (DEFINED "_wasm_debuginfo_${_wasm_debuginfo}")
  list(APPEND emscripten_debug_options
    ${_wasm_debuginfo_${_wasm_debuginfo}})
  list(APPEND emscripten_link_options
    ${_wasm_debuginfo_${_wasm_debuginfo}_link})
else ()
  message (FATAL_ERROR "Unrecognized value for _wasm_debuginfo=${_wasm_debuginfo}")
endif ()

target_compile_options(WrappedAsyncClipper
  PUBLIC
    ${emscripten_compile_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
)

target_link_options(WrappedAsyncClipper
  PUBLIC
    ${emscripten_link_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  WrappedAsyncClipper
  MODULES  ${vtk_libs_used}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET WrappedAsyncClipper
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:WrappedAsyncClipper>
)
