vtk_module_test_data(
  Data/2LYZ.pdb
  Data/Microscopy/small2.ndpi)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtk)

  add_subdirectory(Python)
endif ()
