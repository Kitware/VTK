set(tests
  TestIncludeGL2PS.cxx)

vtk_add_test_cxx(vtkgl2psCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkgl2psCxxTests tests)
