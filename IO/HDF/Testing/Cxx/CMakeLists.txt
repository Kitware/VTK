vtk_add_test_cxx(vtkIOHDFCxxTests tests
  TestHDFReader.cxx,NO_VALID,NO_OUTPUT
  TestHDFReaderTemporal.cxx,NO_VALID,NO_OUTPUT
  TestHDFWriter.cxx,NO_VALID
  TestHDFWriterTemporal.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkIOHDFCxxTests tests)
