vtk_add_test_cxx(vtknetcdfCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeNetCDF.cxx)
vtk_test_cxx_executable(vtknetcdfCxxTests tests)
