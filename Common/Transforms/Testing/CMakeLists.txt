add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/earth.ppm
    Data/headsq/,REGEX:.*
    Data/masonry.bmp)

  add_subdirectory(Python)
endif ()
