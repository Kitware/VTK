vtk_module_test_data(
  Data/uniform-001371-5x5x5.vtp)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/headsq/,REGEX:.*)

  add_subdirectory(Python)
endif ()
