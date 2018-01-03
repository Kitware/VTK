if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/NetCDF/test.pop.nc)

  add_subdirectory(Cxx)
endif ()
