IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
      3DMorph
      banana
      bore
      camera
      clipHex
      clipPyramid
      clipTet
      clipWedge
      cubeAxes
      depthSort
      earth
      imageToPolyData
      financialField2
      largeImageOffset
      largeImageParallel
      MassProperties
      TestGridWarp3D
      TestGridWarpCubic
      TestGridWarpLinear
      TestGridWarpNearest
      TestICPTransform
      TestImageStencilWithPolydata
      TestThinPlateWarp
      TestThinPlateWarp3D
      text
      VRMLImporter
      WarpPolyData
      xyPlot
      xyPlot2
      xyPlot3
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Hybrid/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Hybrid/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    FOREACH ( tfile 
      3DMorph
      banana
      camera
      clipHex
      clipPyramid
      clipTet
      clipWedge
      depthSort
      MassProperties
      TestICPTransform
      text
      WarpPolyData
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Hybrid/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Hybrid/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

