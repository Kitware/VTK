vtk_add_test_cxx(vtkIOAvmeshReaderCxxTests tests
  NO_VALID NO_OUTPUT
  TestAvmeshReader.cxx)
vtk_test_cxx_executable(vtkIOAvmeshReaderCxxTests tests)
