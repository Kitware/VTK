vtk_module_test_data(
  Data/2LYZ.pdb
  Data/Microscopy/small2.ndpi)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtk)

  add_subdirectory(Python)
endif ()
