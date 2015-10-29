vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestPLYReader.cxx
  TestPLYReaderTextureUV.cxx
  TestPLYWriter.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
