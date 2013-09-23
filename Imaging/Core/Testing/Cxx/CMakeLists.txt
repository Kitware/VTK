vtk_add_test_cxx(
  FastSplatter.cxx
  ImageAccumulate.cxx,NO_VALID
  ImageAutoRange.cxx
  ImageBSplineCoefficients.cxx
  ImageHistogram.cxx
  ImageHistogramStatistics.cxx,NO_VALID
  ImageResize.cxx
  ImageResize3D.cxx
  ImageResizeCropping.cxx
  ImageWeightedSum.cxx,NO_VALID
  ImportExport.cxx,NO_VALID # todo (unsatistfied deps)
  TestUpdateExtentReset.cxx,NO_VALID
  )

# The stencil test is special
set_property(DIRECTORY APPEND PROPERTY VTK_TEST_CXX_SOURCES
  TestImageStencilData.cxx
  )
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

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
