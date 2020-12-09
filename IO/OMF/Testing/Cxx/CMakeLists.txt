vtk_add_test_cxx(vtkIOOMFCxxTests tests
  TESTING_DATA
  TestOMFReader.cxx)

vtk_test_cxx_executable(vtkIOOMFCxxTests tests)
