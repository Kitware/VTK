vtk_add_test_cxx(vtkImagingCoreCxxTests tests
  FastSplatter.cxx
  ImageAccumulate.cxx,NO_VALID
  ImageAccumulateLarge.cxx,NO_VALID,NO_DATA,NO_OUTPUT 32
  ImageAutoRange.cxx
  ImageBlend.cxx
  ImageBSplineCoefficients.cxx
  ImageChangeInformation.cxx,NO_VALID,NO_DATA
  ImageDifference.cxx,NO_VALID
  ImageGenericInterpolateSlidingWindow3D.cxx
  ImageHistogram.cxx
  ImageHistogramStatistics.cxx,NO_VALID
  ImageInterpolateSlidingWindow2D.cxx
  ImageInterpolateSlidingWindow3D.cxx
  ImageInterpolator.cxx,NO_VALID,NO_DATA
  ImagePassInformation.cxx,NO_VALID,NO_DATA
  ImageResize.cxx
  ImageResize3D.cxx
  ImageResizeCropping.cxx
  ImageReslice.cxx
  ImageResliceDirection.cxx
  ImageResliceGap.cxx
  ImageResliceOriented.cxx
  ImageWeightedSum.cxx,NO_VALID
  ImportExport.cxx,NO_VALID
  TestBSplineWarp.cxx
  TestImageProbeFilter.cxx
  TestImageStencilDataMethods.cxx,NO_VALID
  TestImageStencilIterator.cxx,NO_VALID
  TestImageSSIM.cxx,NO_VALID
  TestStencilWithLasso.cxx
  TestStencilWithPolyDataContour.cxx
  TestStencilWithPolyDataSurface.cxx
  TestUpdateExtentReset.cxx,NO_VALID)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(_vtk_test_cxx_wasm_enabled_in_browser ON)
endif ()
vtk_add_test_cxx(vtkImagingCoreCxxTests tests
  1
  TestAddStencilData,TestImageStencilData.cxx,TIGHT_VALID)
vtk_add_test_cxx(vtkImagingCoreCxxTests tests
  2
  TestSubtractStencilData,TestImageStencilData.cxx,TIGHT_VALID)
vtk_add_test_cxx(vtkImagingCoreCxxTests tests
  3
  TestClipStencilData,TestImageStencilData.cxx,TIGHT_VALID)

vtk_test_cxx_executable(vtkImagingCoreCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
