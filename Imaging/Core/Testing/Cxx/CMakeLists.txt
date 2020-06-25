vtk_add_test_cxx(vtkImagingCoreCxxTests tests
  FastSplatter.cxx
  ImageAccumulate.cxx,NO_VALID
  ImageAccumulateLarge.cxx,NO_VALID,NO_DATA,NO_OUTPUT 32
  ImageAutoRange.cxx
  ImageBSplineCoefficients.cxx
  ImageHistogram.cxx
  ImageHistogramStatistics.cxx,NO_VALID
  ImageInterpolateSlidingWindow2D.cxx
  ImageInterpolateSlidingWindow3D.cxx
  ImageResize.cxx
  ImageResize3D.cxx
  ImageResizeCropping.cxx
  ImageReslice.cxx
  ImageWeightedSum.cxx,NO_VALID
  ImportExport.cxx,NO_VALID
  TestBSplineWarp.cxx
  TestImageProbeFilter.cxx
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
ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::ImagingCoreCxx-AddStencilData
  COMMAND vtkImagingCoreCxxTests TestImageStencilData 1
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestAddStencilData.png,:})
ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::ImagingCoreCxx-SubtractStencilData
  COMMAND vtkImagingCoreCxxTests TestImageStencilData 2
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestSubtractStencilData.png,:})
ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
  NAME VTK::ImagingCoreCxx-ClipStencilData
  COMMAND vtkImagingCoreCxxTests TestImageStencilData 3
    -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestClipStencilData.png,:})

vtk_test_cxx_executable(vtkImagingCoreCxxTests tests
  RENDERING_FACTORY
  )
