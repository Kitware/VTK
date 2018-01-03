set(classes
  vtkPythonAlgorithm)

vtk_module_add_module(VTK::FiltersPython
  CLASSES ${classes})
# Without linking to libPython directly, loading the library from a C++
# binary causes Python symbols to not be resolved since no Python iterpreter
# has been loaded yet.
vtk_module_set_properties(VTK::FiltersPython
  INTERFACE_vtk_python_direct_link 1)
