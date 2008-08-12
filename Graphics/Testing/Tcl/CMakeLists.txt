IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  #
  # Add tests
  #
  IF (VTK_DATA_ROOT)

    IF (VTK_USE_MANGLED_MESA)
      ADD_TEST(MesaCone-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/MesaCone.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Graphics/MesaCone.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF (VTK_USE_MANGLED_MESA)

    FOREACH ( tfile
      AutomaticPlaneGeneration
      Canny
      CellDataToPointData
      Delaunay2D
      Delaunay2DAlpha
      Delaunay2DTransform
      Delaunay3D
      Disk
      EnSight6Elements
      EnSight6OfficeBin
      EnSightBlow1ASCII
      EnSightBlow1Bin
      EnSightBlow2ASCII
      EnSightBlow2Bin
      EnSightBlow3Bin
      EnSightBlow4Bin
      EnSightBlow5ASCII
      EnSightCompound
      EnSightGoldElements
      EnSightIronProtASCII
      EnSightIronProtBin
      EnSightMandelbrot
      EnSightNfacedASCII
      EnSightNfacedBin
      EnSightOfficeASCII
      EnSightOfficeBin
      EnSightRectGridASCII
      EnSightRectGridBin
      EnSightSelectArrays
      ExtractEdgesQuadraticCells
      ExtractTensors
      Hyper
      HyperScalarBar
      KlineBottle
      LineIntersectQuadraticCells
      MatrixToTransform
      MultidimensionalSolution
      NoLightGeneration
      OBBCylinder
      OSCone
      PolyDataMapperAllPolygons
      PolyDataMapperAllWireframe
      QuadricDecimation
      StreamPolyData
      TenEllip
      TestBandedContourFilter
      TestBoxFunction
      TestButtonSource
      TestCameraInterpolator
      TestCellDerivs
      TestCurvatures
      TestDiscreteMarchingCubes
      TestFillHolesFilter
      TestGraphLayoutFilter
      TestGridSynchronizedTemplates3D
      TestImageMarchingCubes
      TestMarchingSquares
      TestMultiBlockStreamer
      TestParametricFunctions
      TestPlatonicSolids
      TestQuadricClustering
      TestRandomAttributeGenerator
      TestRectilinearGridToTetrahedra
      TestRectilinearSynchronizedTemplates
      TestRegularPolygonSource
      TestRibbonAndTube
      TestRotate
      TestRuledSurface
      TestRuledSurface2
      TestSpherePuzzle
      TestSpherePuzzleArrows
      TestSplineFilter
      TestStructuredGrid
      TestSynchronizedTemplates2D
      TestSynchronizedTemplates3D
      TestTransformCoordinateSystems
      TestTransformInterpolator
      TestTriangleFilter
      TransformConcatenation
      TransformCoverage
      TransformPolyData
      blankGrid
      capCow
      capSphere
      clipArt
      clipComb
      clipImage
      clipQuadraticCells
      clipVolume
      clipVolume2
      clipVolume3
      combStreamers
      combStreamers2
      coneResolution
      constrainedDelaunay
      contour2DAll
      contour3DAll
      contourCells
      contourQuadraticCells
      contoursToSurface
      cowHair
      cowHair2
      createBFont
      cursor2D
      cursor3D
      cutLoop
      cylMap
      cylindrical
      dataSetSurfaceFilter
      deciPlane
      dicer
      dispPlot
      edgePoints
      eleState
      extractPolyData
      extractRectGrid
      extractUGrid
      extractVectors
      extrudeCopyCD
      fieldToPolyData
      fieldToRGrid
      fieldToSGrid
      fieldToUGrid
      financialField
      genHead
      geomFilter
      glyph2D
      headBone
      hull
      iceCream
      imageMCAll
      mcubes
      mergeFilter
      motor
      multipleComponentContour
      multipleIso
      nacaBinary
      officeStreamPoints
      polyConn
      probe
      probeComb
      progGlyphs
      progGlyphsBySource
      quadricCut
      rectGrid
      recursiveDC
      reverseNormals
      scalarColors
      scalarConn
      schwarz
      shepards
      skinOrder
      smoothCyl
      smoothCyl2
      smoothMeshOnMesh
      socbal
      spatialRepAll
      spherical
      splitVectors
      stereoDresdenMace
      streamTracer
      stripF
      subDivideTetra
      subPixelPositionMin
      subdividePointData
      sync3dAll
      teapotHulls
      testDataSetTriangleFilter
      testDataSetTriangleFilter2
      testHexaPenta
      testReflect
      textureThreshold
      triangularTCoords
      triangularTexture
      warplens
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/Graphics/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )

  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    # All rtImageTest tests require VTKData to pass...
    # Do not add any rtImageTest based tests in this block.
  ENDIF (VTK_DATA_ROOT)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

ADD_TEST(PrintSelf-Graphics ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Graphics)

ADD_TEST(TestSetObjectMacro-Graphics ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Graphics/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
