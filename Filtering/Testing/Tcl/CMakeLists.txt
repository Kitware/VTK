IF (HAVE_PNG)
 IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)

    ADD_TEST(CSpline-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/CSpline.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Filtering/CSpline.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)

    ADD_TEST(CSpline-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/CSpline.tcl
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

  ENDIF (VTK_DATA_ROOT)

 ENDIF (VTK_USE_RENDERING)
ENDIF (HAVE_PNG)

