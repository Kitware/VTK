IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
      dem
      TestImageWriters
      TestPolygonWriters
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/IO/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/IO/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )

    IF(PLY_LIBRARY)
      IF(PLY_PATH)
        ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
          ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
          ${VTK_SOURCE_DIR}/IO/Testing/Tcl/TestPLYReadWrite.tcl
          -D ${VTK_DATA_ROOT}
          -V Baseline/IO/TestPLYReadWrite.png
          -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
      ENDIF(PLY_PATH)
    ENDIF(PLY_LIBRARY)

  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

