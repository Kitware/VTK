add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/AMR/Enzo/DD0010/,REGEX:.*)

  add_subdirectory(Python)
endif ()
