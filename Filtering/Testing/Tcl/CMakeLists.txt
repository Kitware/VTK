IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
      closedSplines
      CSpline
      KSpline
      SelectionLoop
      ImplicitSum
      PerlinNoise
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Filtering/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
    IF(VTK_USE_TK)
      ADD_TEST(squadViewer-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/squadViewer.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Filtering/squadViewer.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF(VTK_USE_TK)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-Filtering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Filtering)

ADD_TEST(TestSetObjectMacro-Filtering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Filtering/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
ADD_TEST(otherDataSetAttributes ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/Filtering/Testing/Tcl/otherDataSetAttributes.tcl 
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
