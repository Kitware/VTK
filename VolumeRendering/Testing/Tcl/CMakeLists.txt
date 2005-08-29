IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile
      gaussian
      TestBunykRayCastFunction
      TestFixedPointRayCasterLinear
      TestFixedPointRayCasterNearest
      TestFixedPointRayCasterLinearCropped
      TestFixedPointRayCasterNearestCropped
      TestLODProp3D
      TestPTZSweep
      volRCClipPlanes
      volRCCropRegions
      volRCRotateClip
      volTM2DCropRegions
      volTM2DRotateClip
      volTM3DCropRegions
      volTM3DRotateClip
    )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/VolumeRendering/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/VolumeRendering/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
    IF(VTK_USE_TK)
      ADD_TEST(imageCursor3D-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/VolumeRendering/Testing/Tcl/cursor3D.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/VolumeRendering/cursor3D.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF(VTK_USE_TK)
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-VolumeRendering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/VolumeRendering)

ADD_TEST(TestSetObjectMacro-VolumeRendering ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/VolumeRendering/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
