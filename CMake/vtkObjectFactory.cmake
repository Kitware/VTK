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
