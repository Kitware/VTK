vtk_module_test_data(
  Data/uGridEx.vtk)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
