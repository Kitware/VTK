set(TestGDALRasterNoDataValue_ARGS
  -D DATA{../Data/Input/TestGDALRasterNoDataValue.tif}
)
set(TestGDALRasterPalette_ARGS
  -D DATA{../Data/Input/TestGDALRasterPalette.tif}
)

vtk_add_test_cxx(vtkIOGDALCxxTests tests
  TestGDALVectorReader.cxx
  TestGDALRasterReader.cxx
  TestGDALRasterNoDataValue.cxx,NO_VALID,NO_OUTPUT
  TestGDALRasterPalette.cxx
  )
vtk_test_cxx_executable(vtkIOGDALCxxTests tests)
