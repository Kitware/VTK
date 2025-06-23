vtk_add_test_cxx(vtkIOOMFCxxTests tests
  TESTING_DATA
  TestOMFReader.cxx,NO_SERDES)

vtk_test_cxx_executable(vtkIOOMFCxxTests tests)
