vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestPLYReader.cxx
  TestPLYReaderTextureUV.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
