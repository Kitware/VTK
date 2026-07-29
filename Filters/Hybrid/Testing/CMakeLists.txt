vtk_module_test_data(
  Data/can.ex2
  Data/cow.vtp
  Data/BaselineData/TestTemporalCacheUndefinedTimeStep.vtkhdf)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

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
