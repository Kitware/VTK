vtk_module_test_data(
  Data/uniform-001371-5x5x5.vtp)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/headsq/,REGEX:.*)

  add_subdirectory(Python)
endif ()
