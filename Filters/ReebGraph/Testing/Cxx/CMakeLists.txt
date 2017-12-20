vtk_add_test_cxx(vtkFiltersReebGraphCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestReebGraph.cxx
  )
vtk_test_cxx_executable(vtkFiltersReebGraphCxxTests tests)
