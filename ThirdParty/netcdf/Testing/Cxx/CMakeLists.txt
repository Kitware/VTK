set(tests
  TestIncludeNetCDF.cxx)

vtk_add_test_cxx(vtknetcdfCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtknetcdfCxxTests tests)
