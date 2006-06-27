IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
      dem
      TestAVSucdReader
      TestGAMBITReader
      TestImageWriters
      TestPLYReadWrite
      TestPolygonWriters
      TestJPEGReader
      TestTIFFReader
      TestBMPReader
      TestChacoReader
      Plot3DScalars
      Plot3DVectors
      ParticleReader
      caffeine
      TestXYZMol
      TestMHD
      TestSetFileNames
      TestSimplePointsReader
      TestXMLRectilinearGridIO
      TestXMLStructuredGridIO
      TestXMLImageDataIO
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/IO/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/IO/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
    IF (VTK_USE_PARALLEL)
      FOREACH ( tfile 
        TestXMLPolyDataIO
        TestXMLUnstructuredGridIO
        )
        ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/IO/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/IO/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
      ENDFOREACH( tfile )
    ENDIF (VTK_USE_PARALLEL)
    # tests that don't produce images
    FOREACH ( tfile
      TestGlobFileNames
      TestSortFileNames
      )
      ADD_TEST(${tfile} ${VTK_EXECUTABLE}
               ${VTK_SOURCE_DIR}/IO/Testing/Tcl/${tfile}.tcl)
    ENDFOREACH( tfile )
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(TestEmptyXML ${VTK_EXECUTABLE}
         ${VTK_SOURCE_DIR}/IO/Testing/Tcl/TestEmptyXML.tcl)

ADD_TEST(PrintSelf-IO ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/IO)

ADD_TEST(TestSetObjectMacro-IO ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/IO/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
