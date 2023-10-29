set(tests
  TestIncludeFMT.cxx)

vtk_add_test_cxx(vtkfmtCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkfmtCxxTests tests)
