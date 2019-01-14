if (VTK_USE_LARGE_DATA)
  vtk_module_test_data(
    Data/NetCDF/,REGEX:.*
    Data/SLAC/ll-9cell-f523/ll-9cell-f523.ncdf
    Data/SLAC/ll-9cell-f523/mode0.l0.R2.457036E+09I2.778314E+04.m3
    Data/SLAC/pic-example/,REGEX:.*\.mod
    Data/SLAC/pic-example/,REGEX:.*\.ncdf
    Data/SLAC/pillbox/,REGEX:.*)

  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/sampleCurveGrid4.nc
    Data/sampleGenGrid3.nc
    Data/tos_O1_2001-2002.nc
    )

  add_subdirectory(Python)
endif ()
