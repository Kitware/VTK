vtk_module_test_data(
  Data/ImageDiff1.png
  Data/ImageDiff2.png
  Data/ImageSSIM.vti
  Data/beach.tif
  Data/fullhead15.png
  Data/headsq/,REGEX:.*
  Data/ssim.vti)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

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
