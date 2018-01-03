if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/LSDyna/ale/,REGEX:.*
    Data/LSDyna/foam/,REGEX:.*
    Data/LSDyna/hemi.draw/,REGEX:.*
    Data/LSDyna/bug_14713/,REGEX:.*
    Data/LSDyna/impact/,REGEX:.*)

  add_subdirectory(Cxx)
endif ()
