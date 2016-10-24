vtk_add_test_cxx(${vtk-module}CxxTests tests
  FastSplatter.cxx
  ImageAccumulate.cxx,NO_VALID
  ImageAccumulateLarge.cxx,NO_VALID,NO_DATA,NO_OUTPUT 32
  ImageAutoRange.cxx
  ImageBSplineCoefficients.cxx
  ImageHistogram.cxx
  ImageHistogramStatistics.cxx,NO_VALID
  ImageResize.cxx
  ImageResize3D.cxx
  ImageResizeCropping.cxx
  ImageReslice.cxx
  ImageWeightedSum.cxx,NO_VALID
  ImportExport.cxx,NO_VALID
  TestBSplineWarp.cxx
  TestImageStencilDataMethods.cxx,NO_VALID
  TestImageStencilIterator.cxx,NO_VALID
  TestStencilWithLasso.cxx
  TestStencilWithPolyDataContour.cxx
  TestStencilWithPolyDataSurface.cxx
  TestUpdateExtentReset.cxx,NO_VALID
  )
list(APPEND tests
  TestImageStencilData.cxx
  )

# The stencil test is special
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-AddStencilData
  COMMAND ${vtk-module}CxxTests TestImageStencilData 1
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V DATA{../Data/Baseline/TestAddStencilData.png,:})
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-SubtractStencilData
  COMMAND ${vtk-module}CxxTests TestImageStencilData 2
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V DATA{../Data/Baseline/TestSubtractStencilData.png,:})
ExternalData_add_test(VTKData
  NAME ${vtk-module}Cxx-ClipStencilData
  COMMAND ${vtk-module}CxxTests TestImageStencilData 3
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V DATA{../Data/Baseline/TestClipStencilData.png,:})

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
