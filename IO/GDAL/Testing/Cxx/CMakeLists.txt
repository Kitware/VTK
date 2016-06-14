set(TestGDALRasterNoDataValue_ARGS
  -D DATA{../Data/Input/TestGDALRasterNoDataValue.tif}
)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestGDALVectorReader.cxx
  TestGDALRasterReader.cxx
  TestGDALRasterNoDataValue.cxx,NO_VALID,NO_OUTPUT
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
