INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      # Add test converted from Tcl
      SET( tests
        gaussian
        TestBunykRayCastFunction
        #TestFixedPointRayCasterLinear -- source file
        TestFixedPointRayCasterNearest
        #TestFixedPointRayCasterLinearCropped -- source file
        #TestFixedPointRayCasterNearestCropped -- source file
        TestLODProp3D
        TestPTZSweep
        volRCClipPlanes
        volRCCropRegions
        volRCRotateClip
        volTM2DCropRegions
        volTM2DRotateClip
        #VolumePicker
        )

      CONVERT_TCL_TEST_TO_PY (
        tests
        VolumeRendering)

     

    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
