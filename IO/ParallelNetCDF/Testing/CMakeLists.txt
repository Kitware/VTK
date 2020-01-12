if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/NetCDF/test.pop.nc
    Data/SLAC/ll-9cell-f523/ll-9cell-f523.ncdf
    Data/SLAC/ll-9cell-f523/mode0.l0.R2.457036E+09I2.778314E+04.m3)
  add_subdirectory(Cxx)
endif ()
