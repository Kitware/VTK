vtk_module_test_data(
  Data/cow.vtp)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/SainteHelens.dem
    Data/clown.facet
    Data/earth.ppm
    Data/headsq/,REGEX:.*
    Data/iflamigm.3ds
    Data/masonry.bmp
    Data/vtk.png)

  add_subdirectory(Python)
endif ()
