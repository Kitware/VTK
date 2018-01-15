if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/XDMF/Iron/,REGEX:Iron_Protein\.ImageData\..*)

  add_subdirectory(Cxx)
endif ()
