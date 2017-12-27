vtk_add_test_cxx(vtkIOPLYCxxTests tests
  TestPLYReader.cxx
  TestPLYReaderIntensity.cxx
  TestPLYReaderPointCloud.cxx
  TestPLYReaderTextureUV.cxx
  TestPLYWriterAlpha.cxx
  TestPLYWriter.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOPLYCxxTests tests)
