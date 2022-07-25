# Create a list of all the wrapped module names
set(_vtk_python_wrapped_module_names)
foreach (_vtk_python_module IN LISTS vtk_python_wrapped_modules)
  get_property(_vtk_python_library_name
    TARGET    "${_vtk_python_module}"
    PROPERTY  "INTERFACE_vtk_module_library_name")
  list(APPEND _vtk_python_wrapped_module_names "${_vtk_python_library_name}")
endforeach ()

set(python_tests
  TestErrorObserver.py)

if (TARGET vtkpythonmodules_pyi)
  set(TestCompilePYI_ARGS
    "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}/vtkmodules"
    ${_vtk_python_wrapped_module_names})
  list(APPEND python_tests TestCompilePYI.py)
endif()

vtk_add_test_python(
  NO_DATA
  NO_OUTPUT
  NO_VALID
  ${python_tests}
  )
