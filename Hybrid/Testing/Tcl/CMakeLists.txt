IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    FOREACH ( tfile 
      3DMorph
      appendImplicitModel
      appendImplicitModelNoBounds
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
      financialField3
      largeImageOffset
      largeImageParallel
      MassProperties
      OverlayTextOnImage
      TestCaptionActor
      TestGreedyTerrainDecimation
      TestGridWarp3D
      TestGridWarpCubic
      TestGridWarpLinear
      TestGridWarpNearest
      TestICPTransform
      TestImageStencilWithPolydata
      TestInteractorEventRecorder
      TestPCA
      TestProcrustes
      TestThinPlateWarp
      TestThinPlateWarp3D
      text
      VRMLImporter
      WarpPolyData
      xyPlot
      xyPlot2
      xyPlot3
      xyPlot4
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Hybrid/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Hybrid/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH( tfile )
    IF(NOT VTK_USE_COCOA)
      ADD_TEST(cursor3D-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Hybrid/Testing/Tcl/cursor3D.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Hybrid/cursor3D.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF(NOT VTK_USE_COCOA)
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
      TestPCA
      TestProcrustes
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

ADD_TEST(PrintSelf-Hybrid ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Hybrid)

ADD_TEST(TestSetObjectMacro-Hybrid ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
	 "${VTK_SOURCE_DIR}/Hybrid/vtk\\\\*.h"
	 "vtkSetObjectMacro")
