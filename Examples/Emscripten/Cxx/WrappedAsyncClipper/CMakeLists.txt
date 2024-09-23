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
find_package(VTK
  COMPONENTS
    CommonColor
    FiltersGeneral
    FiltersSources
    InteractionStyle
    InteractionWidgets
    RenderingOpenGL2
    RenderingUI)

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------
add_executable(WrappedAsyncClipper WrappedAsyncClipper.cxx WrappedAsyncClipper.h)
target_compile_features(WrappedAsyncClipper PRIVATE cxx_std_14) # for initialized lambda captures
target_link_libraries(WrappedAsyncClipper PRIVATE ${VTK_LIBRARIES})

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

target_compile_options(WrappedAsyncClipper
  PUBLIC
    ${emscripten_compile_options}
)

target_link_options(WrappedAsyncClipper
  PUBLIC
    ${emscripten_link_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  WrappedAsyncClipper
  MODULES  ${VTK_LIBRARIES}
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
