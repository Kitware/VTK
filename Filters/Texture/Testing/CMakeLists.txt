if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/bluntfinq.bin
    Data/bluntfinxyz.bin
    Data/earth.ppm
    Data/masonry.bmp
    Data/texThres2.vtk)

  add_subdirectory(Python)
endif ()
