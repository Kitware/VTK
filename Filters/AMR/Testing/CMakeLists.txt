if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/AMR/Enzo/DD0010/,REGEX:.*)

  add_subdirectory(Python)
endif ()
