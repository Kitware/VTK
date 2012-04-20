create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestImageStencilData.cxx
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

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)
list(REMOVE_ITEM TestsToRun TestImageStencilData.cxx)

# The stencil test is special
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

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Imaging/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
