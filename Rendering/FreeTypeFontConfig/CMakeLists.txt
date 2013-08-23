find_package(FontConfig REQUIRED)

include_directories(${FONTCONFIG_INCLUDE_DIR})

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

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(${vtk-module} ${Module_SRCS})

target_link_libraries(${vtk-module} LINK_PRIVATE ${FONTCONFIG_LIBRARIES})
