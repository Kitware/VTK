find_package(FontConfig REQUIRED)

include_directories(${FONTCONFIG_INCLUDE_DIR})

include(vtkObjectFactory)

set(Module_SRCS
  vtkFontConfigFreeTypeTools.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/vtkRenderingFreeTypeFontConfigObjectFactory.cxx
)

set_source_files_properties(
  vtkFontConfigFreeTypeTools
  vtkRenderingFreeTypeFontConfigObjectFactory
  WRAP_EXCLUDE
  )

# Setup overrides
list(APPEND vtk_module_overrides "vtkFreeTypeTools")
set(vtk_module_vtkFreeTypeTools_override "vtkFontConfigFreeTypeTools")
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(${vtk-module} ${Module_SRCS})

vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${FONTCONFIG_LIBRARIES})
