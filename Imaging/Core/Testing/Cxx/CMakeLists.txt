vtk_add_test_cxx(Imaging
  # FastSplatter.cxx # todo (unsatistfied deps)
  ImageAccumulate.cxx
  # ImageAutoRange.cxx # todo (unsatistfied deps)
  ImageBSplineCoefficients.cxx
  # ImageHistogram.cxx # todo (unsatistfied deps)
  # ImageHistogramStatistics.cxx # todo (unsatistfied deps)
  # ImageResize.cxx # todo (unsatistfied deps)
  # ImageResize3D.cxx # todo (unsatistfied deps)
  # ImageResizeCropping.cxx # todo (unsatistfied deps)
  ImageWeightedSum.cxx
  # ImportExport.cxx # todo (unsatistfied deps)
  TestUpdateExtentReset.cxx
  )

# The stencil test is special
set_property(DIRECTORY APPEND PROPERTY VTK_TEST_CXX_SOURCES
  TestImageStencilData.cxx
  )
if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-AddStencilData
    COMMAND ${vtk-module}CxxTests TestImageStencilData 1
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/Hybrid/TestAddStencilData.png)
  add_test(NAME ${vtk-module}Cxx-SubtractStencilData
    COMMAND ${vtk-module}CxxTests TestImageStencilData 2
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/Hybrid/TestSubtractStencilData.png)
  add_test(NAME ${vtk-module}Cxx-ClipStencilData
    COMMAND ${vtk-module}CxxTests TestImageStencilData 3
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/Hybrid/TestClipStencilData.png)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)
