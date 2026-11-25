cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(CustomShading)

# SlangKit is linked by VTK itself, so it needs no find_package() here: VTK finds it again.
find_package(VTK
  COMPONENTS
    CommonComputationalGeometry
    CommonCore
    FiltersCore
    FiltersSources
    InteractionStyle
    RenderingCore
    RenderingSlang
    RenderingUI
    RenderingWebGPU)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(CustomShading CustomShading.cxx)
target_link_libraries(CustomShading
  PRIVATE
    ${VTK_LIBRARIES})
# The example loads custom_shading.slang and custom_palette.slang from here by default.
target_compile_definitions(CustomShading
  PRIVATE
    "CUSTOM_SHADING_MODULE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
vtk_module_autoinit(
  TARGETS CustomShading
  MODULES ${VTK_LIBRARIES})

if (BUILD_TESTING)
  include(CTest)
  add_test(NAME CustomShading COMMAND CustomShading --test)
  # Skipped when VTK has no SlangKit.
  set_tests_properties(CustomShading PROPERTIES SKIP_RETURN_CODE 125)
endif ()
