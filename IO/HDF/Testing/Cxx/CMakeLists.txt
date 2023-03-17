vtk_add_test_cxx(vtkIOHDFCxxTests tests
  TestHDFReader.cxx,NO_VALID,NO_OUTPUT
  TestHDFReaderTransient.cxx,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkIOHDFCxxTests tests)
