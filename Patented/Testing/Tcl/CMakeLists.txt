IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
      dcubes
      deciFran
      mcubes
      TestSynchronizedTemplates2D
      TestSynchronizedTemplates3D
      TestGridSynchronizedTemplates3D
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Patented/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Patented/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

