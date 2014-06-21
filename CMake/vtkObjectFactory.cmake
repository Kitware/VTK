# This file provides several helper macros/functions to aid in overrideing
# the VTK object factories. These methods should be used to ensure that certain
# interface classes have valid overrides provided in implementation modules.
# If the interface class doesn't use one of the object factory based new macros
# in C++ then the overrides will not work as expected.

# Add a new override, base is the base class name and override is the name of
# the class that should override. For example,
# vtk_add_override(vtkRenderer vtkOpenGLRenderer)
macro(vtk_add_override base override)
  list(APPEND vtk_module_overrides ${base})
  set(vtk_module_${base}_override ${override})
endmacro()

# Create the relevant object factory files from the override list. This uses
# ${vtk-module} to construct the file names (${vtk-module}ObjectFactory.[h,cxx]
# in the current binary directory.
function(vtk_object_factory_configure override_list)
  # Now we iterate and create that class file...
  foreach(_class ${override_list})
    set(_override ${vtk_module_${_class}_override})
    set(_vtk_override_includes
      "${_vtk_override_includes}\n#include \"${_override}.h\"")
    set(_vtk_override_creates "${_vtk_override_creates}
  VTK_CREATE_CREATE_FUNCTION(${_override})")
    set(_vtk_override_do "${_vtk_override_do}
    this->RegisterOverride(\"${_class}\",
                           \"${_override}\",
                           \"Override for ${vtk-module} module\", 1,
                           vtkObjectFactoryCreate${_override});")
  endforeach()
  string(TOUPPER ${vtk-module} VTK-MODULE)
  configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
  configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
endfunction()
