cmake_minimum_required(VERSION 3.13)
project(AsyncClipper)

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
add_executable(AsyncClipper AsyncClipper.cxx)

target_link_libraries(AsyncClipper PRIVATE ${VTK_LIBRARIES})

# -----------------------------------------------------------------------------
# Emscripten compile+link options
# -----------------------------------------------------------------------------
set(emscripten_link_options)
list(APPEND emscripten_link_options
  "-pthread"
  "-sEXPORTED_RUNTIME_METHODS=['ENV']"
  "-sPROXY_TO_PTHREAD=1"
  "-sALLOW_MEMORY_GROWTH=1"
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

target_compile_options(AsyncClipper
  PUBLIC
    ${emscripten_compile_options}
)

target_link_options(AsyncClipper
  PUBLIC
    ${emscripten_link_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  AsyncClipper
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET AsyncClipper
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:AsyncClipper>
)
