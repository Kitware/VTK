vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestGDALVectorReader.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
