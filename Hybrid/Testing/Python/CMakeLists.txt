INCLUDE(${VTK_SOURCE_DIR}/Utilities/vtkTclTest2Py/vtkConvertTclTestToPy.cmake)
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    IF (VTK_PYTHON_EXE)
      # Add test converted from Tcl
      SET( tests
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
        #financialField2 -- arrays
        #financialField3 -- arrays
        largeImageOffset
        largeImageParallel
        #MassProperties -- complicated global/local mess up 
        OverlayTextOnImage
        TestCaptionActor
        TestFacetReader
        TestGreedyTerrainDecimation
        TestGridWarp3D
        TestGridWarpCubic
        TestGridWarpLinear
        TestGridWarpNearest
        #TestICPTransform  --complex arrays
        TestImageStencilWithPolydata
        TestPCA
        TestProcrustes
        TestProjectedTerrainPath
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
      CONVERT_TCL_TEST_TO_PY (
        tests
        Hybrid)       

    ENDIF (VTK_PYTHON_EXE)
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
