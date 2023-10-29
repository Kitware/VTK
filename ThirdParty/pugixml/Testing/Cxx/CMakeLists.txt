set(tests
  TestIncludePugiXml.cxx)

vtk_add_test_cxx(vtkpugixmlCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkpugixmlCxxTests tests)
