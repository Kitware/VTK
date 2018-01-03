vtk_module_test_data(
  Data/beach.tif
  Data/fullhead15.png
  Data/headsq/,REGEX:.*)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/B.pgm
    Data/E07733S002I009.MR
    Data/cactus.3337.pts
    Data/earth.ppm
    Data/ironProt.vtk
    Data/masonry.bmp
    Data/mr.001
    Data/nut.slc
    )

  add_subdirectory(Python)
endif ()
