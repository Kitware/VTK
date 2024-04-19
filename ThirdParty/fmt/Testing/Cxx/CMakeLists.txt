vtk_add_test_cxx(vtkfmtCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeFMT.cxx)
vtk_test_cxx_executable(vtkfmtCxxTests tests)
