vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestPLYReader.cxx
  TestPLYReaderIntensity.cxx
  TestPLYReaderPointCloud.cxx
  TestPLYReaderTextureUV.cxx
  TestPLYWriter.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
