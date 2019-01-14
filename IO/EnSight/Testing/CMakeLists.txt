if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/EnSight/,REGEX:.*)

  add_subdirectory(Python)
endif ()
