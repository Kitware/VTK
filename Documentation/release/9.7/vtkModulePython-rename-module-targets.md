## vtkModuleWrapPython renames Python module targets

The `vtk_module_wrap_python` CMake API now creates targets with `Py` as the
prefix. This is to avoid conflicts between the Python module target for module
`Namespace::Utils` and the target for the `Namespace::UtilsPython` targets.
