vtk_module_test_data(
  Data/beach.jpg)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
