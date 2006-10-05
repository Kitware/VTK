IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)

    FOREACH ( tfile
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Infovis/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Infovis/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )

  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    FOREACH ( tfile
      )
      ADD_TEST(${tfile} ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Infovis/Testing/Tcl/${tfile}.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-Infovis ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Infovis)

ADD_TEST(TestSetObjectMacro-Infovis ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Infovis/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
