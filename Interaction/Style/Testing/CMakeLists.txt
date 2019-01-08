if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/SainteHelens.dem
    Data/beach.tif)

  add_subdirectory(Python)
endif ()
