if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/LSDyna/hemi.draw/,REGEX:.*)

  add_subdirectory(Cxx)
endif ()
