IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile
      assembly
      CamBlur
      cells 
      ImageActor
      labeledContours
      multiLineText
      pickCells
      rendererSource
      rotations
      ScalarBar
      TestFlyTo
      TestStyleJoystickActor
      TestStyleJoystickCamera
      TestStyleTrackball
      TestStyleTrackballActor
      TestStyleTrackballCamera
      volRCClipPlanes
      volRCCropRegions
      volRCRotateClip
      volTM2DClipPlanes
      volTM2DCropRegions
      volTM2DRotateClip
    )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Rendering/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Rendering/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

