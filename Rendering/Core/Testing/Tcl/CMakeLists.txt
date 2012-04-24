unset(tests)

if(VTK_DATA_ROOT)
  list(APPEND tests
    assembly
    CamBlur
    cells
    ImageActor
    ImageActorStressed
    PickerWithLocator
    ScalarBar
    SurfacePickerWithTexture
    TestLeaderActor2D
    TestOnePoint
    TestOpacity2
    TestTriangleStripCellColor
    TestWindowToImageTransparency
    TexturedSphere
    pickCells
    propAssembly
    rendererSource
    rotations
    )
endif()

foreach(test ${tests})
  set(_test_args
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcl
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  if(VTK_DATA_ROOT)
    list(APPEND _test_args
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/Rendering/${test}.png)
  endif()
  add_test(NAME ${vtk-module}Tcl-${test}
    COMMAND ${VTK_TCL_EXE} ${_test_args})
endforeach()

if(VTK_DATA_ROOT)
  set(tests
    cowHair2
    cowHair
    NoLightGeneration
    PolyDataMapperAllPolygons
    PolyDataMapperAllWireframe
    TestCameraInterpolator
    TestRotate
    TestTransformCoordinateSystems
    TestTransformInterpolator
    TransformConcatenation
    TransformCoverage
    TransformImageData
    TransformPolyData
    stereoDresdenMace
    )
endif()

foreach(test ${tests})
  set(_test_args
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcl
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  if(VTK_DATA_ROOT)
    list(APPEND _test_args
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/Graphics/${test}.png)
  endif()
  add_test(NAME ${vtk-module}Tcl-${test}
    COMMAND ${VTK_TCL_EXE} ${_test_args})
endforeach()
