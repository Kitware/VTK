# Tests with test images in Baseline/Rendering
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    TestStyleJoystickCamera
    TestFlyTo
    TestStyleRubberBandZoom
    TestInteractorStyleTerrain
    TestStyleTerrain
    TestStyleTrackballActor
    TestStyleBaseSpike
    TestStyleTrackballCamera
    TestStyleJoystickActor
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Rendering/${tfile}.png
   )
endforeach()
