IF (VTK_USE_RENDERING)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)

    IF (VTK_USE_MANGLED_MESA)
      ADD_TEST(MesaCone-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/MesaCone.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Graphics/MesaCone.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF (VTK_USE_MANGLED_MESA)

    FOREACH ( tfile
      AutomaticPlaneGeneration
      blankGrid
      Canny
      capCow
      capSphere
      CellDataToPointData
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
      contourCells
      contourQuadraticCells
      contoursToSurface
      contour2DAll
      contour3DAll
      cowHair
      cowHair2
      createBFont
      cursor3D
      cutLoop
      cylindrical
      cylMap
      dataSetSurfaceFilter
      deciPlane
      Delaunay2D
      Delaunay2DAlpha
      Delaunay2DTransform
      Delaunay3D
      dicer
      Disk
      dispPlot
      edgePoints
      eleState
      EnSight6Elements
      EnSight6OfficeBin
      EnSightGoldElements
      EnSightBlow1ASCII
      EnSightBlow1Bin
      EnSightBlow2ASCII
      EnSightBlow2Bin
      EnSightBlow3Bin
      EnSightBlow4Bin
      EnSightBlow5ASCII
      EnSightIronProtASCII
      EnSightIronProtBin
      EnSightOfficeASCII
      EnSightOfficeBin
      EnSightRectGridASCII
      EnSightRectGridBin
      EnSightMandelbrot
      EnSightSelectArrays
      EnSightCompound
      ExtractEdgesQuadraticCells
      extractPolyData
      extractUGrid
      extractRectGrid
      ExtractTensors
      extractVectors
      fieldToPolyData
      fieldToRGrid
      fieldToSGrid
      fieldToUGrid
      geomFilter
      glyph2D
      hull
      Hyper
      HyperScalarBar
      KlineBottle
      LineIntersectQuadraticCells
      motor
      MatrixToTransform
      mergeFilter
      MultidimensionalSolution
      multipleIso
      nacaBinary
      NoLightGeneration
      OBBCylinder
      officeStreamPoints
      OSCone
      polyConn
      PolyDataMapperAllPolygons
      PolyDataMapperAllWireframe
      probeComb
      progGlyphs
      progGlyphsBySource
      quadricCut
      QuadricDecimation
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
      StreamPolyData
      streamTracer
      stripF
      subdividePointData
      subDivideTetra
      subPixelPositionMin
      teapotHulls
      TenEllip
      TestBandedContourFilter
      TestBoxFunction
      TestButtonSource
      TestCellDerivs
      TestCurvatures
      testDataSetTriangleFilter
      testDataSetTriangleFilter2
      TestGraphLayoutFilter
      TestMultiBlockStreamer
      TestPlatonicSolids
      TestQuadricClustering
      TestRectilinearGridToTetrahedra
      testReflect
      TestRuledSurface
      TestRuledSurface2
      TestSpherePuzzle
      TestSpherePuzzleArrows
      TestSplineFilter
      TestStructuredGrid
      TestTriangleFilter
      TestRibbonAndTube
      textureThreshold
      TransformConcatenation
      TransformCoverage
      TransformPolyData
      triangularTCoords
      triangularTexture
      warplens
      )
      ADD_TEST(${tfile}-image ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl
        -D ${VTK_DATA_ROOT}
        -V Baseline/Graphics/${tfile}.png
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )

  IF(VTK_USE_HYBRID)
    ADD_TEST(extrudeCopyCD-image ${VTK_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
      ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/extrudeCopyCD.tcl
      -D ${VTK_DATA_ROOT}
      -V Baseline/Graphics/extrudeCopyCD.png
      -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
  ENDIF(VTK_USE_HYBRID)

  #
  # If we do not have the data, still run the tests that we can
  #
  ELSE (VTK_DATA_ROOT)
    IF (VTK_USE_MANGLED_MESA)
      ADD_TEST(MesaCone ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/MesaCone.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDIF (VTK_USE_MANGLED_MESA)

    FOREACH ( tfile
      capSphere
      CellDataToPointData
      clipVolume
      clipVolume2
      clipVolume3
      coneResolution
      constrainedDelaunay
      contourCells
      contoursToSurface
      cutLoop
      Delaunay2D
      Delaunay2DAlpha
      Delaunay2DTransform
      Disk
      extractPolyData
      glyph2D
      Hyper
      KlineBottle
      MatrixToTransform
      OBBCylinder
      progGlyphs
      progGlyphsBySource
      quadricCut
      scalarConn
      schwarz
      shepards
      smoothCyl
      smoothCyl2
      subdividePointData
      subDivideTetra
      TestBoxFunction
      TestGraphLayoutFilter
      TestQuadricDecimation
      triangularTCoords
      triangularTexture
      warplens
      TestGraphLayoutFilter
      TransformConcatenation
      TransformCoverage
      TransformPolyData
      )
      ADD_TEST(${tfile} ${VTK_EXECUTABLE}
        ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl 
        ${VTK_SOURCE_DIR}/Graphics/Testing/Tcl/${tfile}.tcl
        -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
    ENDFOREACH ( tfile )
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)

ADD_TEST(PrintSelf-Graphics ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl  
         ${VTK_SOURCE_DIR}/Graphics)

ADD_TEST(TestSetObjectMacro-Graphics ${TCL_TCLSH}
         ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl
         "${VTK_SOURCE_DIR}/Graphics/vtk\\\\*.h"
         "vtkSetObjectMacro"
         )
